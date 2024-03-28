/**
 * Parsers for booking names scraped from NSS
 * These are tried in order so make sure if one is a subset of another, it comes first
 */
import { NameParser } from "./types";


const PARSERS: Record<string, NameParser> = {
  // Matches standard classes
  // - 3 chars form the class type
  // - A bunch of characters before the separator
  // - Then the class name
  // Examples:
  //   TUT/14:1 COMM1150
  //   LEC/A:2 <13-16> - 2 LEC comb CHEM1811
  //   LA2/04:1 SOMS1912
  CLASS:
  {
    pattern: /(?<type>[A-Z0-9]{3})[^\u00a0]*\u00a0(?<name>.*)/,
    parser: (matchGroups) => ({ bookingType: 'CLASS', name: matchGroups['name'] + ' ' + matchGroups['type'] })
  },

  // Matches Arc society bookings
  // - Starts with 'StuGrp', then separator
  // - Name of society
  // Examples:
  //   StuGrp DEBATING-005
  //   StuGrp ACTUARIALSOC-003
  SOCIETY:
  {
    pattern: /StuGrp[^\u00a0]*\u00a0(?<name>[^-]*)/,
    parser: (matchGroups) => ({ bookingType: 'SOCIETY', name: matchGroups['name'] })
  },

  // Matches society bookings that are done internally
  // - Starts with 'Misc', then separator
  // - Followed by * and a date
  // - ARCSG followed by name of society
  // - Numbers at the end for each distinct booking of soc
  // Examples:
  //   Misc *20230307-ARCSGCSE-001
  //   Misc *20230405-ARCSGWOMENINENGINEERING-002
  INTERNAL_SOCIETY:
  {
    pattern: /Misc[^\u00a0]*\u00a0\*\d{8}-ARCSG(?<name>[A-Z]*)/,
    parser: (matchGroups) => ({ bookingType: 'SOCIETY', name: matchGroups['name'] })
  },

  // Matches bookings from faculty and UNSW internal programs
  // - As with INTERNAL_SOCIETY, but it doesn't have ARCSG
  // Examples:
  //   Misc *20230404-FUTURESTUDENTS-001
  //   Misc *20230206-HUMSLANG-001
  INTERNAL:
  {
    pattern: /Misc[^\u00a0]*\u00a0\*\d{8}-(?<name>[A-Z]*)/,
    parser: (matchGroups) => ({ bookingType: 'INTERNAL', name: matchGroups['name'] })
  },

  // Matches 'External' bookings (idk what that means)
  // - Starts with 'External', then separator, then *
  // - Contains course code of booking (then term and id which is ignored)
  // Examples:
  //   External *COMP2521-T1U1-001
  //   External *CHEM4506-T1U1-003
  EXTERNAL:
  {
    pattern: /External[^\u00a0]*\u00a0\*(?<name>[A-Z0-9]*)/,
    parser: (matchGroups) => ({ bookingType: 'INTERNAL', name: matchGroups['name'] })
  },

  // Matches OWeek bookings
  // - Starts with Misc
  // - Some mess, then "OWeek" and a building/term/room
  // Examples:
  //   Misc >*C0602-1002EROweekColomboT2
  //   Misc >*C0602-1002EROweekCLBT1CLB6a
  OWEEK:
  {
    pattern: /Misc[^\u00a0]*\u00a0.*Oweek.*(?<term>T[1-3])/,
    parser: (matchGroups) => ({ bookingType: 'MISC', name: 'OWeek' + matchGroups['term'] })
  },

  // Matches MISC events that are for a class
  // - Literally the same as EXTERNAL but starts with misc
  // Examples:
  //   Misc *MATH1141-T1U1-004
  //   Misc *FINS5516-T1P1-001
  MISC_CLASS:
  {
    pattern: /Misc[^\u00a0]*\u00a0\*(?<name>[A-Z0-9]*)/,
    parser: (matchGroups) => ({ bookingType: 'MISC', name: matchGroups['name'] })
  },

  // Matches Exams bookings
  // - Starts with Misc then separator
  // - SuppExams or Exams then the term
  // Examples:
  //   Misc >*C2205-2605JM2023SuppExamsT1-001c
  //   Misc >*C0708-2508JM2023ExamsT2-005
  EXAMS: 
  {
    pattern: /Misc[^\u00a0]*\u00a0.*\d(?<name>SuppExams|Exams)(?<term>T[1-3])/,
    parser: (matchGroups) => ({ bookingType: 'MISC', name: matchGroups['name'] + ' ' + matchGroups['term'] })
  },

  // Other Misc bookings
  // - Starts with Misc then separator
  // - Two chars, then hyphen, then the name
  // Examples:
  //   Misc C1-EstateManagement-001
  //   Misc WY-ANGLICAN-016a
  MISC:
  {
    pattern: /Misc[^\u00a0]*\u00a0.{2}-(?<name>[^-]*)/,
    parser: (matchGroups) => ({ bookingType: 'MISC', name: matchGroups['name'] })
  },

  // Matches blocker bookings that restrict bookings being made at certain times
  // - Starts with 'Misc' and contains 'Block'
  // - Possibly also contains a reason e.g. weekend, fri night
  // Examples:
  //   Misc >*C0201-0101JMRichieBlockFriNights
  //   Misc >*C0201-0101JMRichieBlockWeekends
  BLOCK:
  {
    pattern: /Misc.*(?<reason>Block|Weekend|Weekday|Fri|FriNight)/i,
    parser: (matchGroups) => ({ bookingType: 'BLOCK', name: matchGroups['reason'] })
  },
}

export default PARSERS;
