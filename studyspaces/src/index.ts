import { buildRows } from "./build.js";
import { SQL } from "./sql.js";
import { batchInsert } from "./hasuragres.js";

async function main() {
    const { buildings, rooms } = await buildRows();

    await batchInsert([
        {
            metadata: {
                table_name: "Buildings",
                columns: ["id", "name", "lat", "long", "aliases"],
                write_mode: "append",
                sql_up: SQL.Buildings.up,
                sql_down: SQL.Buildings.down,
            },
            payload: buildings,
        },
        {
            metadata: {
                table_name: "Rooms",
                columns: ["id", "name", "abbr", "usage", "capacity", "school", "buildingId", "microphone",
                    "accessibility", "audiovisual", "infotechnology", "writingMedia", "service",
                    "lat", "long"],
                write_mode: "append",
                sql_up: SQL.Rooms.up,
                sql_down: SQL.Rooms.down,
            },
            payload: rooms,
        },
    ]);
}


main().catch(err => {
    console.error(err);
    process.exit(1)
});