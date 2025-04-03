import React from "react";
import mockData from "../mockData";

const SpecConformityTable = () => {
  return (
    <table>
      <thead>
        <tr>
          <th>Parameter</th>
          <th>Expected</th>
          <th>Got</th>
          <th>Result</th>
        </tr>
      </thead>
      <tbody>
        {mockData.specConformity.map((item, index) => (
          <tr key={index} className={item.result === "Fail" ? "fail" : "pass"}>
            <td>{item.parameter}</td>
            <td>{item.expected}</td>
            <td>{item.got}</td>
            <td>{item.result}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default SpecConformityTable;

