/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsSelectionMgr.h"

#include <sstream>
#include <string>
#include <Scrap.h>
#include <TextEdit.h>

NS_IMPL_ADDREF(nsDialog)
NS_IMPL_RELEASE(nsDialog)

nsSelectionMgr::nsSelectionMgr()
{
  NS_INIT_REFCNT();

  mCopyStream = 0;
}

nsSelectionMgr::~nsSelectionMgr()
{
}

nsresult nsSelectionMgr::GetCopyOStream(ostream** aStream)
{
  if (mCopyStream)
    delete mCopyStream;
  mCopyStream = new stringstream;
  *aStream = mCopyStream;
  return NS_OK;
}

nsresult nsSelectionMgr::CopyToClipboard()
{
  // we'd better already have a stream ...
  if (!mCopyStream)
      return NS_ERROR_NOT_INITIALIZED;

  string      theString = mCopyStream->str();
  PRInt32     len = theString.length();
  const char* str = theString.data();

  if (len)
  {
    char * ptr = NS_CONST_CAST(char*,str);
    for (PRInt32 plen = len; plen > 0; plen --, ptr ++)
      if (*ptr == '\n')
        *ptr = '\r';

    OSErr err = ::ZeroScrap();
    err = ::PutScrap(len, 'TEXT', str);
    ::TEFromScrap();
  }

  delete mCopyStream;
  mCopyStream = 0;
  return NS_OK;
}

nsresult NS_NewSelectionMgr(nsISelectionMgr** aInstancePtrResult)
{
  nsSelectionMgr* sm = new nsSelectionMgr();
  return sm->QueryInterface(nsISelectionMgr::IID(),
                            (void**) aInstancePtrResult);
}


