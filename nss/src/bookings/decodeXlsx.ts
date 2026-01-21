import xlsx from "xlsx";
import { BookingsExcelRow } from "../types";

// Decodes the buffer data from fetchXlsx
// - Reads the data in as a buffer
// - Extracts relevant rows into BookingExcelRow
const decodeXlsx = (data: Buffer): BookingsExcelRow[] => {
  // Setup the excel sheet
  const workbook = xlsx.read(data, { type: "buffer" });
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

  return bookingRows;
};

export default decodeXlsx;
