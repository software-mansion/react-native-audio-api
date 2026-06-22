import type { BannerZone } from "@swmansion/t-rex-ui";

export const TOP_BAR_BANNER = {
  rotateIntervalMs: 4000,
  hiddenPaths: ["/react-native-audio-api/docs"] as string[],
  zones: [
    {
      zoneId: "react-native-audio-api-topbar-1",
      contentId: "ea15c4216158c4097b65fe6504a4b3b7",
      fallbackBgColor: "#ff6259",
    },
    {
      zoneId: "react-native-audio-api-topbar-2",
      contentId: "ea15c4216158c4097b65fe6504a4b3b7",
      fallbackBgColor: "#ff6259",
    },
    {
      zoneId: "react-native-audio-api-topbar-3",
      contentId: "ea15c4216158c4097b65fe6504a4b3b7",
      fallbackBgColor: "#ff6259",
    },
  ] satisfies BannerZone[],
};
