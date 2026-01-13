import { chromium } from "playwright";
import { BOOKING_LOADING_TIMEOUT } from "./config";

const fetchXlsx = async (url: string, filename: string) => {
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
    console.log("No bookings shown for", url);
    return;
  }
  console.log("Waiting for download");
  const downloadPromise = page.waitForEvent("download");
  await page.click(".e-excel-export button");
  const download = await downloadPromise;

  // Save file
  await download.saveAs(`${filename}.xlsx`);
  console.log(`Download successful! Saving to ${filename}.xlsx`);

  // Close browser
  console.log("Closing browser");
  await zoomedBrowser.close();
  await browser.close();
};

export default fetchXlsx;
