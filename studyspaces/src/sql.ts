import fs from "fs";
import path from "path";
import { fileURLToPath } from 'url';


const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPO_SQL_DIR = path.resolve(__dirname, '..', '..', 'sql');


function readSql(subdir: "buildings" | "rooms" | "bookings", file: "up.sql" | "down.sql"): string {
    const p = path.join(REPO_SQL_DIR, subdir, file);
    return fs.readFileSync(p, "utf8")
}

export const SQL = {
    "Buildings": {
        up: readSql("buildings", "up.sql"),
        down: readSql("buildings", "down.sql"),
    },
    "Rooms": {
        up: readSql("rooms", "up.sql"),
        down: readSql("rooms", "down.sql"),
    },
    "Bookings": {
        up: readSql("bookings", "up.sql"),
        down: readSql("bookings", "down.sql"),
    },
} as const;