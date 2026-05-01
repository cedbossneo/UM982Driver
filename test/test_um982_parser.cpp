// Copyright 2026 Mowgli Project
//
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <string>

#include "mowgli_unicore_gnss/um982_parser.hpp"
#include <gtest/gtest.h>

namespace mowgli_unicore_gnss
{
namespace
{

uint32_t crc32_unicore(const std::string& text)
{
  uint32_t crc = 0U;
  for (const unsigned char ch : text)
  {
    crc ^= static_cast<uint32_t>(ch);
    for (int bit = 0; bit < 8; ++bit)
    {
      const bool lsb = (crc & 1U) != 0U;
      crc >>= 1U;
      if (lsb)
      {
        crc ^= 0xEDB88320U;
      }
    }
  }
  return crc;
}

std::string make_nmea(std::string payload)
{
  unsigned int checksum = 0U;
  for (const unsigned char ch : payload)
  {
    checksum ^= static_cast<unsigned int>(ch);
  }

  char checksum_text[3];
  std::snprintf(checksum_text, sizeof(checksum_text), "%02X", checksum);
  return "$" + payload + "*" + checksum_text;
}

std::string make_unicore(std::string payload)
{
  char crc_text[9];
  std::snprintf(crc_text, sizeof(crc_text), "%08x", crc32_unicore(payload));
  return "#" + payload + "*" + crc_text;
}

}  // namespace

TEST(Um982Parser, ParsesGgaFix)
{
  Um982Parser parser;
  const auto parsed =
      parser.parse_line(make_nmea("GNGGA,123519,4807.038,N,01131.000,E,4,12,0.8,545.4,M,46.9,M,,"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->fix.has_value());
  EXPECT_EQ(parsed->sentence_type, "GGA");
  EXPECT_TRUE(parsed->fix->valid_fix);
  EXPECT_NEAR(parsed->fix->latitude_deg, 48.1173, 1e-6);
  EXPECT_NEAR(parsed->fix->longitude_deg, 11.5166667, 1e-6);
  EXPECT_NEAR(parsed->fix->altitude_m, 592.3, 1e-6);
  EXPECT_EQ(parsed->fix->fix_quality, 4);
  EXPECT_EQ(parsed->fix->satellites, 12);
  EXPECT_NEAR(parsed->fix->hdop, 0.8, 1e-6);
}

TEST(Um982Parser, ParsesHprHeading)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_nmea("GNHPR,235959.00,123.45,-1.25,0.50"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->heading.has_value());
  EXPECT_EQ(parsed->sentence_type, "HPR");
  EXPECT_NEAR(parsed->heading->heading_deg, 123.45, 1e-6);
  ASSERT_TRUE(parsed->heading->pitch_deg.has_value());
  ASSERT_TRUE(parsed->heading->roll_deg.has_value());
  EXPECT_NEAR(*parsed->heading->pitch_deg, -1.25, 1e-6);
  EXPECT_NEAR(*parsed->heading->roll_deg, 0.50, 1e-6);
}

TEST(Um982Parser, ParsesPvtslnaFixWithRtkFixed)
{
  // Field index 8 (data field 1 after a 7-field Unicore header — see
  // kPvtslnaPositionTypeIndex) carries the BESTPOSA-style position-type.
  // "NARROW_INT" maps to GGA quality 4 (RTK Fixed). Indices in the
  // sentence below: 0=PVTSLNA, 1-6=header placeholders, 7=sol_status,
  // 8=pos_type, 9=data[2], 10=altitude, 11=lat, 12=lon, 13-15=stddevs.
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_unicore(
      "PVTSLNA,foo,bar,baz,0,0,0,SOL_COMPUTED,NARROW_INT,0,101.25,48.12345678901,"
      "2.34567890123,0.30,0.10,0.20"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->fix.has_value());
  EXPECT_EQ(parsed->sentence_type, "PVTSLNA");
  EXPECT_TRUE(parsed->fix->valid_fix);
  EXPECT_EQ(parsed->fix->fix_quality, 4);
  EXPECT_NEAR(parsed->fix->latitude_deg, 48.12345678901, 1e-12);
  EXPECT_NEAR(parsed->fix->longitude_deg, 2.34567890123, 1e-12);
  EXPECT_NEAR(parsed->fix->altitude_m, 101.25, 1e-12);
  EXPECT_TRUE(parsed->fix->has_covariance);
  EXPECT_NEAR(parsed->fix->covariance[0], 0.04, 1e-12);
  EXPECT_NEAR(parsed->fix->covariance[4], 0.01, 1e-12);
  EXPECT_NEAR(parsed->fix->covariance[8], 0.09, 1e-12);
}

TEST(Um982Parser, ParsesPvtslnaFloatRtk)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_unicore(
      "PVTSLNA,foo,bar,baz,0,0,0,SOL_COMPUTED,NARROW_FLOAT,0,101.25,48.0,2.0,0.3,0.1,0.2"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->fix.has_value());
  EXPECT_TRUE(parsed->fix->valid_fix);
  EXPECT_EQ(parsed->fix->fix_quality, 5);  // float RTK -> NMEA quality 5
}

TEST(Um982Parser, ParsesPvtslnaNumericPositionType)
{
  // Some firmware variants emit BESTPOSA position-type as numeric code
  // ("50" = NARROW_INT) instead of the string form. Position-type lives
  // at index 8 (cf. kPvtslnaPositionTypeIndex).
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_unicore(
      "PVTSLNA,foo,bar,baz,0,0,0,0,50,0,101.25,48.0,2.0,0.3,0.1,0.2"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->fix.has_value());
  EXPECT_TRUE(parsed->fix->valid_fix);
  EXPECT_EQ(parsed->fix->fix_quality, 4);
}

TEST(Um982Parser, ParsesPvtslnaNoFixWhenPositionTypeNone)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_unicore(
      "PVTSLNA,foo,bar,baz,0,0,0,0,NONE,0,101.25,48.0,2.0,0.3,0.1,0.2"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->fix.has_value());
  EXPECT_FALSE(parsed->fix->valid_fix);
  EXPECT_EQ(parsed->fix->fix_quality, 0);
}

TEST(Um982Parser, ParsesGsvSatellitesInView)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line(
      make_nmea("GPGSV,3,1,12,01,40,083,46,02,17,308,41,03,52,210,42,04,71,047,46"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->gsv.has_value());
  EXPECT_EQ(parsed->sentence_type, "GSV");
  EXPECT_EQ(parsed->gsv->talker, "GP");
  EXPECT_EQ(parsed->gsv->satellites_in_view, 12);
}

TEST(Um982Parser, ParsesGsvPerConstellationTalkers)
{
  Um982Parser parser;
  const auto gl = parser.parse_line(make_nmea("GLGSV,1,1,07"));
  const auto ga = parser.parse_line(make_nmea("GAGSV,1,1,10"));
  const auto gb = parser.parse_line(make_nmea("GBGSV,1,1,08"));

  ASSERT_TRUE(gl.has_value() && gl->gsv.has_value());
  ASSERT_TRUE(ga.has_value() && ga->gsv.has_value());
  ASSERT_TRUE(gb.has_value() && gb->gsv.has_value());
  EXPECT_EQ(gl->gsv->talker, "GL");
  EXPECT_EQ(gl->gsv->satellites_in_view, 7);
  EXPECT_EQ(ga->gsv->talker, "GA");
  EXPECT_EQ(ga->gsv->satellites_in_view, 10);
  EXPECT_EQ(gb->gsv->talker, "GB");
  EXPECT_EQ(gb->gsv->satellites_in_view, 8);
}

TEST(Um982Parser, ParsesBestnavaVelocity)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line(make_unicore("BESTNAVA,foo,bar,3.5,90.0,-0.4,0.2,0.1"));

  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->velocity.has_value());
  EXPECT_EQ(parsed->sentence_type, "BESTNAVA");
  EXPECT_NEAR(parsed->velocity->east_mps, 3.5, 1e-6);
  EXPECT_NEAR(parsed->velocity->north_mps, 0.0, 1e-6);
  EXPECT_NEAR(parsed->velocity->up_mps, -0.4, 1e-6);
  EXPECT_NEAR(parsed->velocity->horizontal_std_mps, 0.1, 1e-6);
  EXPECT_NEAR(parsed->velocity->vertical_std_mps, 0.2, 1e-6);
}

TEST(Um982Parser, RejectsBadChecksum)
{
  Um982Parser parser;
  const auto parsed = parser.parse_line("$GPHDT,10.0,T*00");
  EXPECT_FALSE(parsed.has_value());
}

}  // namespace mowgli_unicore_gnss
