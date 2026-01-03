import fs from "fs";
import xlsx from "xlsx";
import { BookingsExcelRow } from "./types";

const decodeXlsx = (path: string): BookingsExcelRow[] => {
  // Setup the excel sheet
  const workbook = xlsx.readFile(path);
  const sheet = workbook.Sheets[workbook.SheetNames[0]];
  const rows = xlsx.utils.sheet_to_json(sheet);

  // Decode the rows
  const bookingRows = rows.map((row: any) => {
    return {
      module_code: row["Module Code"],
      module_description: row["Module Description"],
      name: row["Name"],
      class_size: row["Class Size"],
      day: row["Day"],
      duration: row["Duration"],
      start_time: row["Start time"],
      end_time: row["End time"],
      dates: row["Dates"],
      allocated_location_name: row["Allocated Location Name"],
      weeks: row["Weeks"],
    };
  });

  // Delete the file after reading it
  fs.unlinkSync(path);

  return bookingRows;
};

export default decodeXlsx;
