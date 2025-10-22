export const BASE = "https://www.learningenvironments.unsw.edu.au";
export const LIST_URL = `${BASE}/physical-spaces/study-spaces`;


function getEnv(name: string): string {
    const v = process.env[name];
    if (!v) throw new Error(`Missing env var : ${name}`);
    return v;
}

export const HASURAGRES_URL = getEnv("HASURAGRES_URL");
export const HASURAGRES_API_KEY = getEnv("HASURAGRES_API_KEY");