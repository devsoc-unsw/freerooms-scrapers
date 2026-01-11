const REGEX_ROOM_IDS = /(K-[A-Z][0-9]{2}-[0-9]{3})/g

const parseRoomIds = (allocated_location_name: string): string[] => {
    return Array.from(allocated_location_name.matchAll(REGEX_ROOM_IDS)).map((match) => match[0])
}
