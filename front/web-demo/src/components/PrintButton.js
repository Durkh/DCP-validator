import React from "react";
import { jsPDF } from "jspdf";

const PrintButton = () => {
  const handlePrint = () => {
    const doc = new jsPDF();
    doc.text("DCP Validation Report", 10, 10);
    doc.text("Generated Report", 10, 20);
    doc.save("report.pdf");
  };

  return <button onClick={handlePrint}>Print Report</button>;
};

export default PrintButton;

