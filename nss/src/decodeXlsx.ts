import * as XLSX from "xlsx";
import { BookingsExcelRow } from "./types";

const decodeXlsx = (path: string): BookingsExcelRow[] => {
  const workbook = XLSX.readFile(path);
  const sheet = workbook.Sheets[workbook.SheetNames[0]];

  const rows = XLSX.utils.sheet_to_json(sheet);

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
      allocated_location_name: row["Allocated Location Name"],
    };
  });

  return bookingRows;
};

export default decodeXlsx;