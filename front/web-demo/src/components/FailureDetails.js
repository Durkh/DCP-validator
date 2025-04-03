import React from "react";
import mockData from "../mockData";

const FailureDetails = () => {
  return (
    <div>
      <h3>Failure Details</h3>
      <ul>
        {mockData.failureDetails.map((detail, index) => (
          <li key={index}>{detail}</li>
        ))}
      </ul>
    </div>
  );
};

export default FailureDetails;

