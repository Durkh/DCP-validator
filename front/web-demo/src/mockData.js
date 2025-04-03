const mockData = {
  electricalInfo: [
    { parameter: "Bus Max Speed", value: "117MHz" },
    { parameter: "VIH (High-level input voltage)", value: "3.3V" },
    { parameter: "VIL (Low-level input voltage)", value: "0.7V" },
    { parameter: "Ileak (Leakage current)", value: "20 µA" },
    { parameter: "Rise Time", value: "1µs" },
    { parameter: "Falling Time", value: "0.2µs" },
    { parameter: "Cycle Time", value: "1.5µs" }
  ],
  specConformity: [
    { parameter: "Speed Test", expected: "4MHz", got: "4MHz", result: "Pass" },
    { parameter: "Rising Time", expected: "1 µs", got: "1 µs", result: "Pass" },
    { parameter: "High Time", expected: "40 µs", got: "40.32 µs", result: "Pass" },
    { parameter: "Falling Time", expected: "1 µs", got: "1 µs", result: "Pass" },
    { parameter: "Low Time", expected: "20 µs", got: "19.73µ", result: "Pass" },
    { parameter: "Sync burst", expected: "-", got: "-", result: "Pass" },
    { parameter: "Sync Time", expected: "250µs", got: "200µs", result: "Fail" },
    { parameter: "Bus Yield", expected: "Yes", got: "No", result: "Fail" }
  ],
  failureDetails: [
    "Sync Time did not hold the bus low for the specified time.",
    "The device failed to yield the bus in a collision scenario."
  ]
};

export default mockData;

