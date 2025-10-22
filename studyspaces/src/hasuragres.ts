import { HASURAGRES_API_KEY, HASURAGRES_URL } from "./config.js";

async function post(pathname: string, body: unknown) {
    const res = await fetch(`${HASURAGRES_URL}${pathname}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-API-Key": HASURAGRES_API_KEY,
        },
        body: JSON.stringify(body),
    });

    if (!res.ok) {
        const t = await res.text().catch(() => "");
        throw new Error(`Hasuragres ${pathname} ${res.status} ${t}`);
    }
}


export function batchInsert(payloads: any[]) {
    return post("/batch_insert", payloads);
}