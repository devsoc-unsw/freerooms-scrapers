import axios from "axios";
import Bottleneck from "bottleneck";
import { MAX_CONCURRENT_REQUESTS, MIN_TIME_MS_BETWEEN_REQUESTS } from "../config";
import { RateLimitedAxiosInstance } from "../types";

const API_BASE_URL =
  "https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public";

const publishAPIClient = axios.create({
  baseURL: API_BASE_URL,
}) as RateLimitedAxiosInstance;

const rateLimiter = new Bottleneck({
  maxConcurrent: MAX_CONCURRENT_REQUESTS,
  minTime: MIN_TIME_MS_BETWEEN_REQUESTS,
});

publishAPIClient.rateLimitedPost = (url, payload) => {
  return rateLimiter.schedule(() => publishAPIClient.post(url, payload));
};
publishAPIClient.rateLimitedGet = (url) => {
  return rateLimiter.schedule(() => publishAPIClient.get(url));
};

export default publishAPIClient;
