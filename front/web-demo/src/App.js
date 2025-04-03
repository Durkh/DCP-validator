import React from "react";
import EditableFields from "./components/EditableFields";
import ElectricalInfoTable from "./components/ElectricalInfoTable";
import SpecConformityTable from "./components/SpecConformityTable";
import FailureDetails from "./components/FailureDetails";
import PrintButton from "./components/PrintButton";
import "./styles.css";

const App = () => {
  return (
    <div className="container">
      <h1>DCP Validation Report</h1>
      <EditableFields />
      <h2>Electrical Information</h2>
      <ElectricalInfoTable />
      <h2>Specification Conformity</h2>
      <SpecConformityTable />
      <FailureDetails />
      <PrintButton />
    </div>
  );
};

export default App;

