/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MP4_DEMUXER_H264_H_
#define MP4_DEMUXER_H264_H_

#include "mp4_demuxer/DecoderData.h"

namespace mp4_demuxer
{

class BitReader;

struct SPSData
{
  /* Decoded Members */
  /*
    pic_width is the decoded width according to:
    pic_width = ((pic_width_in_mbs_minus1 + 1) * 16)
                - (frame_crop_left_offset + frame_crop_right_offset) * 2
   */
  uint32_t pic_width;
  /*
    pic_height is the decoded height according to:
    pic_height = (2 - frame_mbs_only_flag) * ((pic_height_in_map_units_minus1 + 1) * 16)
                 - (frame_crop_top_offset + frame_crop_bottom_offset) * 2
   */
  uint32_t pic_height;

  bool interlaced;

  /*
   Displayed size.
   display_width and display_height are adjusted according to the display
   sample aspect ratio.
   */
  uint32_t display_width;
  uint32_t display_height;

  float sample_ratio;

  uint32_t crop_left;
  uint32_t crop_right;
  uint32_t crop_top;
  uint32_t crop_bottom;

  /*
    H264 decoding parameters according to ITU-T H.264 (T-REC-H.264-201402-I/en)
   http://www.itu.int/rec/T-REC-H.264-201402-I/en
   */

  bool constraint_set0_flag;
  bool constraint_set1_flag;
  bool constraint_set2_flag;
  bool constraint_set3_flag;
  bool constraint_set4_flag;
  bool constraint_set5_flag;

  /*
    profile_idc and level_idc indicate the profile and level to which the coded
    video sequence conforms when the SVC sequence parameter set is the active
    SVC sequence parameter set.
   */
  uint8_t profile_idc;
  uint8_t level_idc;

  /*
    seq_parameter_set_id identifies the sequence parameter set that is referred
    to by the picture parameter set. The value of seq_parameter_set_id shall be
    in the range of 0 to 31, inclusive.
   */
  uint8_t seq_parameter_set_id;

  /*
    chroma_format_idc specifies the chroma sampling relative to the luma
    sampling as specified in clause 6.2. The value of chroma_format_idc shall be
    in the range of 0 to 3, inclusive. When chroma_format_idc is not present,
    it shall be inferred to be equal to 1 (4:2:0 chroma format).
    When profile_idc is equal to 183, chroma_format_idc shall be equal to 0
    (4:0:0 chroma format).
   */
  uint8_t chroma_format_idc;

  /*
    separate_colour_plane_flag equal to 1 specifies that the three colour
    components of the 4:4:4 chroma format are coded separately.
    separate_colour_plane_flag equal to 0 specifies that the colour components
    are not coded separately. When separate_colour_plane_flag is not present,
    it shall be inferred to be equal to 0. When separate_colour_plane_flag is
    equal to 1, the primary coded picture consists of three separate components,
    each of which consists of coded samples of one colour plane (Y, Cb or Cr)
    that each use the monochrome coding syntax. In this case, each colour plane
    is associated with a specific colour_plane_id value.
   */
  bool separate_colour_plane_flag;

  /*
    log2_max_frame_num_minus4 specifies the value of the variable
    MaxFrameNum that is used in frame_num related derivations as
    follows:

     MaxFrameNum = 2( log2_max_frame_num_minus4 + 4 ). The value of
    log2_max_frame_num_minus4 shall be in the range of 0 to 12, inclusive.
   */
  uint8_t log2_max_frame_num;

  /*
    pic_order_cnt_type specifies the method to decode picture order
    count (as specified in subclause 8.2.1). The value of
    pic_order_cnt_type shall be in the range of 0 to 2, inclusive.
   */
  uint8_t pic_order_cnt_type;

  /*
    log2_max_pic_order_cnt_lsb_minus4 specifies the value of the
    variable MaxPicOrderCntLsb that is used in the decoding
    process for picture order count as specified in subclause
    8.2.1 as follows:

    MaxPicOrderCntLsb = 2( log2_max_pic_order_cnt_lsb_minus4 + 4 )

    The value of log2_max_pic_order_cnt_lsb_minus4 shall be in
    the range of 0 to 12, inclusive.
   */
  uint8_t log2_max_pic_order_cnt_lsb;

  /*
    delta_pic_order_always_zero_flag equal to 1 specifies that
    delta_pic_order_cnt[ 0 ] and delta_pic_order_cnt[ 1 ] are
    not present in the slice headers of the sequence and shall
    be inferred to be equal to 0.
   */
  bool delta_pic_order_always_zero_flag;

  /*
    offset_for_non_ref_pic is used to calculate the picture
    order count of a non-reference picture as specified in
    8.2.1. The value of offset_for_non_ref_pic shall be in the
    range of -231 to 231 - 1, inclusive.
   */
  int8_t offset_for_non_ref_pic;

  /*
    offset_for_top_to_bottom_field is used to calculate the
    picture order count of a bottom field as specified in
    subclause 8.2.1. The value of offset_for_top_to_bottom_field
    shall be in the range of -231 to 231 - 1, inclusive.
   */
  int8_t offset_for_top_to_bottom_field;

  /*
    max_num_ref_frames specifies the maximum number of short-term and
    long-term reference frames, complementary reference field pairs,
    and non-paired reference fields that may be used by the decoding
    process for inter prediction of any picture in the
    sequence. max_num_ref_frames also determines the size of the sliding
    window operation as specified in subclause 8.2.5.3. The value of
    max_num_ref_frames shall be in the range of 0 to MaxDpbSize (as
    specified in subclause A.3.1 or A.3.2), inclusive.
   */
  uint32_t max_num_ref_frames;

  /*
    gaps_in_frame_num_value_allowed_flag specifies the allowed
    values of frame_num as specified in subclause 7.4.3 and the
    decoding process in case of an inferred gap between values of
    frame_num as specified in subclause 8.2.5.2.
   */
  bool gaps_in_frame_num_allowed_flag;

  /*
    pic_width_in_mbs_minus1 plus 1 specifies the width of each
    decoded picture in units of macroblocks.  16 macroblocks in a row
   */
  uint32_t pic_width_in_mbs;

  /*
    pic_height_in_map_units_minus1 plus 1 specifies the height in
    slice group map units of a decoded frame or field.  16
    macroblocks in each column.
   */
  uint32_t pic_height_in_map_units;

  /*
    frame_mbs_only_flag equal to 0 specifies that coded pictures of
    the coded video sequence may either be coded fields or coded
    frames. frame_mbs_only_flag equal to 1 specifies that every
    coded picture of the coded video sequence is a coded frame
    containing only frame macroblocks.
   */
  bool frame_mbs_only_flag;

  /*
    mb_adaptive_frame_field_flag equal to 0 specifies no
    switching between frame and field macroblocks within a
    picture. mb_adaptive_frame_field_flag equal to 1 specifies
    the possible use of switching between frame and field
    macroblocks within frames. When mb_adaptive_frame_field_flag
    is not present, it shall be inferred to be equal to 0.
   */
  bool mb_adaptive_frame_field_flag;

  /*
    frame_cropping_flag equal to 1 specifies that the frame cropping
    offset parameters follow next in the sequence parameter
    set. frame_cropping_flag equal to 0 specifies that the frame
    cropping offset parameters are not present.
   */
  bool frame_cropping_flag;
  uint32_t frame_crop_left_offset;;
  uint32_t frame_crop_right_offset;
  uint32_t frame_crop_top_offset;
  uint32_t frame_crop_bottom_offset;

  // VUI Parameters

  /*
    vui_parameters_present_flag equal to 1 specifies that the
    vui_parameters( ) syntax structure as specified in Annex E is
    present. vui_parameters_present_flag equal to 0 specifies that
    the vui_parameters( ) syntax structure as specified in Annex E
    is not present.
   */
  bool vui_parameters_present_flag;

  /*
   aspect_ratio_info_present_flag equal to 1 specifies that
   aspect_ratio_idc is present. aspect_ratio_info_present_flag
   equal to 0 specifies that aspect_ratio_idc is not present.
   */
  bool aspect_ratio_info_present_flag;

  /*
    aspect_ratio_idc specifies the value of the sample aspect
    ratio of the luma samples. Table E-1 shows the meaning of
    the code. When aspect_ratio_idc indicates Extended_SAR, the
    sample aspect ratio is represented by sar_width and
    sar_height. When the aspect_ratio_idc syntax element is not
    present, aspect_ratio_idc value shall be inferred to be
    equal to 0.
   */
  uint8_t aspect_ratio_idc;
  uint32_t sar_width;
  uint32_t sar_height;

  /*
    video_signal_type_present_flag equal to 1 specifies that video_format,
    video_full_range_flag and colour_description_present_flag are present.
    video_signal_type_present_flag equal to 0, specify that video_format,
    video_full_range_flag and colour_description_present_flag are not present.
   */
  bool video_signal_type_present_flag;

  /*
    overscan_info_present_flag equal to1 specifies that the
    overscan_appropriate_flag is present. When overscan_info_present_flag is
    equal to 0 or is not present, the preferred display method for the video
    signal is unspecified (Unspecified).
   */
  bool overscan_info_present_flag;
  /*
    overscan_appropriate_flag equal to 1 indicates that the cropped decoded
    pictures output are suitable for display using overscan.
    overscan_appropriate_flag equal to 0 indicates that the cropped decoded
    pictures output contain visually important information in the entire region
    out to the edges of the cropping rectangle of the picture
   */
  bool overscan_appropriate_flag;

  /*
    video_format indicates the representation of the pictures as specified in
    Table E-2, before being coded in accordance with this
    Recommendation | International Standard. When the video_format syntax element
    is not present, video_format value shall be inferred to be equal to 5.
    (Unspecified video format)
   */
  uint8_t video_format;

  /*
    video_full_range_flag indicates the black level and range of the luma and
    chroma signals as derived from E???Y, E???PB, and E???PR or E???R, E???G, and E???B
    real-valued component signals.
    When the video_full_range_flag syntax element is not present, the value of
    video_full_range_flag shall be inferred to be equal to 0.
   */
  bool video_full_range_flag;

  /*
    colour_description_present_flag equal to1 specifies that colour_primaries,
    transfer_characteristics and matrix_coefficients are present.
    colour_description_present_flag equal to 0 specifies that colour_primaries,
    transfer_characteristics and matrix_coefficients are not present.
   */
  bool colour_description_present_flag;

  /*
    colour_primaries indicates the chromaticity coordinates of the source
    primaries as specified in Table E-3 in terms of the CIE 1931 definition of
    x and y as specified by ISO 11664-1.
    When the colour_primaries syntax element is not present, the value of
    colour_primaries shall be inferred to be equal to 2 (the chromaticity is
    unspecified or is determined by the application).
   */
  uint8_t colour_primaries;

  /*
    transfer_characteristics indicates the opto-electronic transfer
    characteristic of the source picture as specified in Table E-4 as a function
    of a linear optical intensity input Lc with a nominal real-valued range of 0
    to 1.
    When the transfer_characteristics syntax element is not present, the value
    of transfer_characteristics shall be inferred to be equal to 2
    (the transfer characteristics are unspecified or are determined by the
    application).
   */
  uint8_t transfer_characteristics;

  uint8_t matrix_coefficients;
  bool chroma_loc_info_present_flag;
  uint32_t chroma_sample_loc_type_top_field;
  uint32_t chroma_sample_loc_type_bottom_field;
  bool timing_info_present_flag;
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  bool fixed_frame_rate_flag;

  SPSData();
};

class H264
{
public:
  static bool DecodeSPSFromExtraData(const ByteBuffer* aExtraData, SPSData& aDest);
  /* Extract RAW BYTE SEQUENCE PAYLOAD from NAL content.
     Returns nullptr if invalid content.
     This is compliant to ITU H.264 7.3.1 Syntax in tabular form NAL unit syntax
   */
  static already_AddRefed<ByteBuffer> DecodeNALUnit(const ByteBuffer* aNAL);
  /* Decode SPS NAL RBSP and fill SPSData structure */
  static bool DecodeSPS(const ByteBuffer* aSPS, SPSData& aDest);
  // Ensure that SPS data makes sense, Return true if SPS data was, and false
  // otherwise. If false, then content will be adjusted accordingly.
  static bool EnsureSPSIsSane(SPSData& aSPS);

private:
  static void vui_parameters(BitReader& aBr, SPSData& aDest);
  // Read HRD parameters, all data is ignored.
  static void hrd_parameters(BitReader& aBr);
};

} // namespace mp4_demuxer

#endif // MP4_DEMUXER_H264_H_
