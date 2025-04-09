import {fetchReportData, populateDefaultValues} from "result.js"

// Event listener for download PDF button
document.getElementById('print-button').addEventListener('click', function() {
    window.print();
});

// Event listener for fetch data button
document.getElementById('fetch-data-button').addEventListener('click', function() {
    fetchReportData();
});

document.getElementById('device-type').addEventListener('input', function() {

    const deviceType = document.getElementById('device-type').value;
    const deviceSpeed = document.getElementById('device-speed').value;
    
    if (deviceType !== "Selecione" && deviceSpeed !== "Selecione") {
        populateDefaultValues();
    }
});

document.getElementById('device-speed').addEventListener('input', function() {

    const deviceType = document.getElementById('device-type').value;
    const deviceSpeed = document.getElementById('device-speed').value;
    
    if (deviceType !== "Selecione" && deviceSpeed !== "Selecione") {
        populateDefaultValues();
    }
});
