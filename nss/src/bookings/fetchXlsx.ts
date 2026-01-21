import { chromium } from "playwright";
import { BOOKING_LOADING_TIMEOUT } from "../config";

// Fetches booking data by extract the booking data as excel
// - Loads the browsers and waits for bookings to load
// - Downloads the bookings and saves into a buffer
const fetchXlsx = async (url: string): Promise<Buffer> => {
  // Setup page and go to url
  console.log("Opening browser");
  const browser = await chromium.launch({ headless: true });
  const zoomedBrowser = await browser.newContext({
    viewport: { width: 1920, height: 1080 },
  });
  const page = await zoomedBrowser.newPage();
  await page.goto(url);

  // Get click excel button
  await page.waitForSelector(".e-excel-export button");
  try {
    await page.waitForSelector(".e-appointment div", {
      state: "attached",
      timeout: BOOKING_LOADING_TIMEOUT,
    });
  } catch (error) {
    // Note: If bookings are supposed to show up, tweak loading timeout
    await browser.close();
    return Promise.reject(
      "No bookings shown for " +
        url +
        "\nNote: Very low chance this should be happening, please double check theres no bookings via the link. If there is, tweak booking loading timeout"
    );
  }
  console.log("Waiting for download");
  const downloadPromise = page.waitForEvent("download");
  await page.click(".e-excel-export button");
  const download = await downloadPromise;

  // Save data into a buffer
  const stream = await download.createReadStream();
  let buffers = [];
  for await (const data of stream) {
    buffers.push(data);
  }
  const buffer = Buffer.concat(buffers);

  // Close browser
  console.log("Closing browser");
  await browser.close();

  return buffer;
};

export default fetchXlsx;
