use rusty::scraper::sheet_reader::read_xlsx;

fn main() {
    let path = format!("{}/example.xlsx", env!("CARGO_MANIFEST_DIR"));

    match read_xlsx(path) {
        Ok(rows) => {
            for row in rows {
                let _ = dbg!(row);
            }
        },
        Err(err) => {
            dbg!(err);
        }
    }
}
