/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

devtools.lazyRequireGetter(this, "Services");
devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
devtools.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");
devtools.lazyRequireGetter(this, "FramerateFront",
  "devtools/server/actors/framerate", true);

devtools.lazyRequireGetter(this, "L10N",
  "devtools/shared/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORIES",
  "devtools/shared/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/shared/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/shared/profiler/global", true);
devtools.lazyRequireGetter(this, "ThreadNode",
  "devtools/shared/profiler/tree-model", true);
devtools.lazyRequireGetter(this, "CallView",
  "devtools/shared/profiler/tree-view", true);

devtools.lazyImporter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
devtools.lazyImporter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "BarGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "SideMenuWidget",
  "resource:///modules/devtools/SideMenuWidget.jsm");

const RECORDING_DATA_DISPLAY_DELAY = 10; // ms
const FRAMERATE_CALC_INTERVAL = 16; // ms
const FRAMERATE_GRAPH_HEIGHT = 60; // px
const CATEGORIES_GRAPH_HEIGHT = 60; // px
const CATEGORIES_GRAPH_MIN_BARS_WIDTH = 3; // px
const CALL_VIEW_FOCUS_EVENTS_DRAIN = 10; // ms
const GRAPH_SCROLL_EVENTS_DRAIN = 50; // ms
const GRAPH_ZOOM_MIN_TIMESPAN = 20; // ms

// This identifier string is used to tentatively ascertain whether or not
// a JSON loaded from disk is actually something generated by this tool.
// It isn't, of course, a definitive verification, but a Good Enough???
// approximation before continuing the import. Don't localize this.
const PROFILE_SERIALIZER_IDENTIFIER = "Recorded Performance Data";
const PROFILE_SERIALIZER_VERSION = 1;

// The panel's window global is an EventEmitter firing the following events:
const EVENTS = {
  // When a recording is started or stopped, via the `stopwatch` button, or
  // when `console.profile` and `console.profileEnd` is invoked.
  RECORDING_STARTED: "Profiler:RecordingStarted",
  RECORDING_ENDED: "Profiler:RecordingEnded",

  // When a recording is abruptly ended, either because the built-in profiler
  // module is stopped by a third party, or because the recordings list is
  // cleared while there's one in progress.
  RECORDING_LOST: "Profiler:RecordingCancelled",

  // When a recording is displayed in the ProfileView.
  RECORDING_DISPLAYED: "Profiler:RecordingDisplayed",

  // When a new tab is spawned in the ProfileView from a graphs selection.
  TAB_SPAWNED_FROM_SELECTION: "Profiler:TabSpawnedFromSelection",

  // When a new tab is spawned in the ProfileView from a node in the tree.
  TAB_SPAWNED_FROM_FRAME_NODE: "Profiler:TabSpawnedFromFrameNode",

  // When different panels in the ProfileView are shown.
  EMPTY_NOTICE_SHOWN: "Profiler:EmptyNoticeShown",
  RECORDING_NOTICE_SHOWN: "Profiler:RecordingNoticeShown",
  LOADING_NOTICE_SHOWN: "Profiler:LoadingNoticeShown",
  TABBED_BROWSER_SHOWN: "Profiler:TabbedBrowserShown",

  // When a source is shown in the JavaScript Debugger at a specific location.
  SOURCE_SHOWN_IN_JS_DEBUGGER: "Profiler:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "Profiler:SourceNotFoundInJsDebugger"
};

/**
 * The current target and the profiler connection, set by this tool's host.
 */
let gToolbox, gTarget, gFront;

/**
 * Initializes the profiler controller and views.
 */
let startupProfiler = Task.async(function*() {
  yield promise.all([
    PrefObserver.register(),
    EventsHandler.initialize(),
    RecordingsListView.initialize(),
    ProfileView.initialize()
  ]);

  // Profiles may have been created before this tool was opened, e.g. via
  // `console.profile` and `console.profileEnd(). Populate the UI with them.
  for (let recordingData of gFront.finishedConsoleRecordings) {
    let profileLabel = recordingData.profilerData.profileLabel;
    let recordingItem = RecordingsListView.addEmptyRecording(profileLabel);
    RecordingsListView.customizeRecording(recordingItem, recordingData);
  }
  for (let { profileLabel } of gFront.pendingConsoleRecordings) {
    RecordingsListView.handleRecordingStarted(profileLabel);
  }

  // Select the first recording, if available.
  RecordingsListView.selectedIndex = 0;
});

/**
 * Destroys the profiler controller and views.
 */
let shutdownProfiler = Task.async(function*() {
  yield promise.all([
    PrefObserver.unregister(),
    EventsHandler.destroy(),
    RecordingsListView.destroy(),
    ProfileView.destroy()
  ]);
});

/**
 * Observes pref changes on the devtools.profiler branch and triggers the
 * required frontend modifications.
 */
let PrefObserver = {
  register: function() {
    this.branch = Services.prefs.getBranch("devtools.profiler.");
    this.branch.addObserver("", this, false);
  },
  unregister: function() {
    this.branch.removeObserver("", this);
  },
  observe: function(subject, topic, pref) {
    Prefs.refresh();

    if (pref == "ui.show-platform-data") {
      RecordingsListView.forceSelect(RecordingsListView.selectedItem);
    }
  }
};

/**
 * Functions handling target-related lifetime events.
 */
let EventsHandler = {
  /**
   * Listen for events emitted by the current tab target.
   */
  initialize: function() {
    this._onConsoleProfileStart = this._onConsoleProfileStart.bind(this);
    this._onConsoleProfileEnd = this._onConsoleProfileEnd.bind(this);

    gFront.on("profile", this._onConsoleProfileStart);
    gFront.on("profileEnd", this._onConsoleProfileEnd);
    gFront.on("profiler-unexpectedly-stopped", this._onProfilerDeactivated);
  },

  /**
   * Remove events emitted by the current tab target.
   */
  destroy: function() {
    gFront.off("profile", this._onConsoleProfileStart);
    gFront.off("profileEnd", this._onConsoleProfileEnd);
    gFront.off("profiler-unexpectedly-stopped", this._onProfilerDeactivated);
  },

  /**
   * Invoked whenever `console.profile` is called.
   *
   * @param string profileLabel
   *        The provided string argument if available, undefined otherwise.
   */
  _onConsoleProfileStart: function(event, profileLabel) {
    RecordingsListView.handleRecordingStarted(profileLabel);
  },

  /**
   * Invoked whenever `console.profileEnd` is called.
   *
   * @param object recordingData
   *        The profiler and refresh driver ticks data received from the front.
   */
  _onConsoleProfileEnd: function(event, recordingData) {
    RecordingsListView.handleRecordingEnded(recordingData);
  },

  /**
   * Invoked whenever the built-in profiler module is deactivated.
   * @see ProfilerConnection.prototype._onProfilerUnexpectedlyStopped
   */
  _onProfilerDeactivated: function() {
    RecordingsListView.removeForPredicate(e => e.isRecording);
    RecordingsListView.handleRecordingCancelled();
  }
};

/**
 * Shortcuts for accessing various profiler preferences.
 */
const Prefs = new ViewHelpers.Prefs("devtools.profiler", {
  showPlatformData: ["Bool", "ui.show-platform-data"]
});

/**
 * Convenient way of emitting events from the panel window.
 */
EventEmitter.decorate(this);

/**
 * DOM query helpers.
 */
function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
