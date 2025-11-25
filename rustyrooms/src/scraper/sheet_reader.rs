use std::error::Error;

use calamine::{DeError, RangeDeserializerBuilder, Reader, Xlsx, open_workbook};
use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct RemoteSheetRow {
    #[serde(rename = "Module Code")]
    pub module_code: String,
    #[serde(rename = "Module Description")]
    pub module_description: String,
    #[serde(rename = "Name")]
    pub name: String,
    #[serde(rename = "Class Size")]
    pub class_size: i32,
    #[serde(rename = "Day")]
    pub day: String,
    #[serde(rename = "Duration")]
    pub duration: String,
    #[serde(rename = "Start time")]
    pub start_time: String,
    #[serde(rename = "End time")]
    pub end_time: String,
    #[serde(rename = "Dates")]
    pub dates: String,
    #[serde(rename = "Allocated Location Name")]
    pub allocated_location_name: String,
    #[serde(rename = "Source")]
    pub source: String,
}

pub fn read_xlsx(path: String) -> Result<Vec<Result<RemoteSheetRow, DeError>>, Box<dyn Error>> {
    let mut workbook: Xlsx<_> = open_workbook(path)?;
    let range = workbook.worksheet_range("Sheet1")?;
    Ok(
        RangeDeserializerBuilder::with_deserialize_headers::<RemoteSheetRow>()
            .from_range(&range)?
            .collect(),
    )
}
