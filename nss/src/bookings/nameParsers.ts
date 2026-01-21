/**
 * Parsers for booking names scraped from Publish
 * These are tried in order so make sure if one is a subset of another, it comes first
 */
import { NameParser } from "../types";

const PARSERS: Record<string, NameParser> = {
  // Matches MISC events that are for a class
  // - Literally the same as EXTERNAL but starts with misc
  // Examples:
  //   *MATH1141-T1U1-004
  //   *FINS5516-T1P1-001
  //	 FS-B-T2-CA-Business Law
  MISC_CLASS: {
    pattern: /\*(?!Block)(?<name>[A-Z0-9]*)|(FS-(?<name2>[^-].+))/,
    parser: (matchGroups) => ({
      bookingType: "MISC",
      name: matchGroups.name ? matchGroups.name : "FS-" + matchGroups.name2,
    }),
  },

  // Matches standard classes
  // - Starts with the class name, then separator '-'
  // - A bunch of characters before the next separator '-'
  // - 3 chars form the class type
  // Examples:
  //   ARTS2871-T3U1-TUT/02:1
  //   ~COMP9417-T3P1+COMP9417-T3U1-TUT/S8:1
  CLASS: {
    pattern: /(?<name>[A-Z]{4}[0-9]{4})-.*-(?<type>[A-Z0-9]{3})/,
    parser: (matchGroups) => ({
      bookingType: "CLASS",
      name: matchGroups["name"] + " " + matchGroups["type"],
    }),
  },

  // Matches Arc society bookings
  // - Starts with * and a date, then separator '-'
  // - ARC followed by name of society
  // - Numbers at the end for each distinct booking of soc
  // Examples:
  //  *20251103-ARCMEDSOC-003
  //	*20251105-ARCSTAFF-002
  SOCIETY: {
    pattern: /\*\d{8}-ARC(?<name>[A-Z0-9]+)-/,
    parser: (matchGroups) => ({
      bookingType: "SOCIETY",
      name: matchGroups["name"],
    }),
  },

  // Matches society bookings that are done internally
  // - Starts with * and a date
  // - ARCSG followed by name of society
  // - Numbers at the end for each distinct booking of soc
  // Examples:
  //  *20230307-ARCSGCSE-001
  //  *20230405-ARCSGWOMENINENGINEERING-002
  //	*20251104-ARCSGLAW-002
  INTERNAL_SOCIETY: {
    pattern: /\*\d{8}-ARCSG(?<name>[A-Z0-9]+)-/,
    parser: (matchGroups) => ({
      bookingType: "SOCIETY",
      name: matchGroups["name"],
    }),
  },

  // Matches bookings from faculty and UNSW internal programs
  // - As with INTERNAL_SOCIETY, but it doesn't have ARCSG
  // Examples:
  //   *20230404-FUTURESTUDENTS-001
  //   *20230206-HUMSLANG-001
  INTERNAL: {
    pattern: /\*\d{8}-(?<name>[A-Z]*)/,
    parser: (matchGroups) => ({
      bookingType: "INTERNAL",
      name: matchGroups["name"],
    }),
  },

  // DEPRECATED - This type is combined with MISC_CLASS
  // Matches 'External' bookings (idk what that means)
  // - Starts with *, then course code of booking
  // - then term and id which is ignored
  // Examples:
  //   *COMP2521-T1U1-001
  //   *CHEM4506-T1U1-003
  // EXTERNAL: {
  //   pattern: /\*(?<name>[A-Z0-9]*)/,
  //   parser: (matchGroups) => ({
  //     bookingType: "INTERNAL",
  //     name: matchGroups["name"],
  //   }),
  // },

  // Matches blocker bookings that restrict bookings being made at certain times
  // - Starts with 'Block'
  // - Possibly also contains a reason e.g. weekend, fri night
  // Examples:
  //  <*BlockHospitalityClancyFri
  //	<*BlockGraduationsT1-003b
  // 	<*BlockExamsSuppsT3-2025-001b
  //	<*BlockT2C-001
  //	<*BlockExamsT3-004
  //	<*BlockExamsT1-007
  //	<*BlockCSE-LAB-002
  BLOCK: {
    pattern: /^<?\*?Block(?<name>.+)$/i,
    parser: (matchGroups) => {
      let name = matchGroups["name"]?.trim() || "Block";
      // Remove everything from the last dash onwards
      const lastDashIndex = name.lastIndexOf("-");
      if (lastDashIndex !== -1) {
        name = name.substring(0, lastDashIndex);
      }
      return {
        bookingType: "BLOCK",
        name: name || "Block", // Fallback if name becomes empty
      };
    },
  },

  // Matches Exams bookings
  // - Contains Final Exam, Supp Exam, SuppExams or Exams
  // Examples:
  //  ECON1401 Final Exam
  //	ENGG1300 Supp Exam
  //	MATH5525 Supplementary Exam
  // 	Supp Exam MMAN4410 and GSOE9830
  //  SUPP EXAMS
  //
  EXAMS: {
    pattern:
      /(?:(?<name>[A-Z]{4}[0-9]{4})\s*)?(?<reason>Final\s+Exams?|Supp\w*\s+Exams?|Exams)(?:\s+(?<name3>.*))?/i,
    parser: (matchGroups) => {
      if (matchGroups.reason && matchGroups.name3) {
        return {
          bookingType: "EXAMS",
          name: `${matchGroups.reason} ${matchGroups.name3}`.trim(),
        };
      }

      if (matchGroups.name && matchGroups.reason) {
        return {
          bookingType: "EXAMS",
          name: `${matchGroups.name} ${matchGroups.reason}`,
        };
      }

      if (matchGroups.reason) {
        return {
          bookingType: "EXAMS",
          name: matchGroups.reason,
        };
      }

      return {
        bookingType: "EXAMS",
        name: "Exams",
      };
    },
  },

  // Matches OWeek bookings
  // - Starts with "<*", then "block"
  // - Followed by the booker and the location
  // - Finally "O-Week"
  // Examples:
  //   <*BlockV&EClancyO-Week
  OWEEK: {
    pattern: /.*Block(?<name>[^ ]*)O-Week/,
    parser: (matchGroups) => ({
      bookingType: "MISC",
      name: "OWeek" + matchGroups["name"],
    }),
  },

  // Other Misc bookings
  // - Starts with Misc then separator
  // - Two chars, then hyphen, then the name
  // Examples:
  //   C1-EstateManagement-001
  //   WY-ANGLICAN-016a
  MISC: {
    pattern: /^.{2}-(?<name>[^-]+)|^(?<name2>.+)$/,
    parser: (matchGroups) => ({
      bookingType: "MISC",
      name: (matchGroups["name"] || matchGroups["name2"]).trim(),
    }),
  },
};

export function normaliseRoomName(name: string): string {
  return name.split(" ")[0].trim();
}

export default PARSERS;
