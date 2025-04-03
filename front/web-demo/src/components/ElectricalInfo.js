import React from "react";
import mockData from "../mockData";

const ElectricalInfoTable = () => {
  return (
    <table>
      <thead>
        <tr>
          <th>Parameter</th>
          <th>Value</th>
        </tr>
      </thead>
      <tbody>
        {mockData.electricalInfo.map((item, index) => (
          <tr key={index}>
            <td>{item.parameter}</td>
            <td>{item.value}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default ElectricalInfoTable;

