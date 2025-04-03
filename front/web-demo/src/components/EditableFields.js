import React, { useState } from "react";

const EditableFields = () => {
  const [name, setName] = useState("Device Name");
  const [version, setVersion] = useState("1.0");
  const [type, setType] = useState("Target");
  const [speed, setSpeed] = useState("4MHz");

  return (
    <div className="editable-fields">
      <label>Name: <input value={name} onChange={(e) => setName(e.target.value)} /></label>
      <label>Version: <input value={version} onChange={(e) => setVersion(e.target.value)} /></label>
      <label>Type: <input value={type} onChange={(e) => setType(e.target.value)} /></label>
      <label>Speed: <input value={speed} onChange={(e) => setSpeed(e.target.value)} /></label>
    </div>
  );
};

export default EditableFields;

