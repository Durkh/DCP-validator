// result.js - Handles fetching and displaying DCP validation report data

const specDelta = [20, 4, 2.5, 1.25];
const specParameterNames = [
    "Speed", "Bit High Time", "Bit Low Time", "Sync Time", "Bit Sync Time", "Bit Sync High", "Bit Sync Low", "Bus Yield"
];

const specDefaultValues = [
    // Speed (MHz), Bit High (μs), Bit Low (μs), Sync Time (μs), Bit Sync Time (μs), Bit Sync High (μs), Bit Sync Low (μs), Bus Yield
    [4,  3*specDelta[0], 2*specDelta[0], 25*specDelta[0], 15*specDelta[0], 7.5*specDelta[0], 7.5*specDelta[0], "Yes"],
    [20, 3*specDelta[1], 2*specDelta[1], 25*specDelta[1], 15*specDelta[1], 7.5*specDelta[1], 7.5*specDelta[1], "Yes"],
    [32, 3*specDelta[2], 2*specDelta[2], 25*specDelta[2], 15*specDelta[2], 7.5*specDelta[2], 7.5*specDelta[2], "Yes"],
    [64, 3*specDelta[3], 2*specDelta[3], 25*specDelta[3], 15*specDelta[3], 7.5*specDelta[3], 7.5*specDelta[3], "Yes"]
];

async function fetchReportData() {
    const deviceType = document.getElementById('device-type').value;
    const deviceSpeed = document.getElementById('device-speed').value;

    //only fetch if the user selected the params
    if (deviceType === "Selecione" || deviceSpeed === "Selecione") {
        showNotification('Please select both Device Type and Device Speed', 'error');
        return;
    }

    try {
        document.getElementById('loading-indicator').style.display = 'block';

        const requestBody = {
            deviceType: deviceType,
            deviceSpeed: deviceSpeed
        };

        const response = await fetch('http:/dcp-validator/api/v1/validation', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(requestBody)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }

        //get validation result in the response
        const data = await response.json();

        populateElectricalInfo(data.electricalInfo);
        populateTransmissionInfo(data.transmissionInfo);
        populateSpecConformity(data.specConformity);
        
        // Only show failure details if we have failures
        const hasFailures = checkForFailures(data);
        
        if (hasFailures && data.failureDetails && data.failureDetails.length > 0) {
            document.querySelector('.failure-details').style.display = 'block';
            populateFailureDetails(data.failureDetails);
        } else {
            document.querySelector('.failure-details').style.display = 'none';
        }

        document.getElementById('loading-indicator').style.display = 'none';

        showNotification('Data loaded successfully', 'success');
    } catch (error) {
        console.error('Error fetching report data:', error);
        document.getElementById('loading-indicator').style.display = 'none';
        showNotification('Failed to load data from server', 'error');
    }
}

function checkForFailures(data) {
    // Check spec conformity for failures
    if (data.specConformity && data.specConformity.length > 0) {
        for (const item of data.specConformity) {
            if (item.result === "Fail") {
                return true;
            }
        }
    }
    
    // Check transmission info for failures
    if (data.transmissionInfo && data.transmissionInfo.length > 0) {
        for (const item of data.transmissionInfo) {
            if (item.result === "Fail") {
                return true;
            }
        }
    }
    
    return false;
}

function populateElectricalInfo(electricalInfo) {
    if (!electricalInfo || !electricalInfo.length) return;

    const electricalTable = document.getElementById('electrical-info-table').getElementsByTagName('tbody')[0];
    electricalTable.innerHTML = '';

    electricalInfo.forEach(item => {
        const row = electricalTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.textContent = item.parameter;
        cell2.textContent = item.value;
    });
}

function populateTransmissionInfo(transmissionInfo) {
    if (!transmissionInfo || !transmissionInfo.length) return;

    const transmissionTable = document.getElementById('transmission-info-table').getElementsByTagName('tbody')[0];
    transmissionTable.innerHTML = '';

    transmissionInfo.forEach(item => {
        const row = transmissionTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.textContent = item.parameter;
        cell2.textContent = item.result;
        
        if (item.result === "Pass") {
            cell2.className = "pass";
        } else if (item.result === "Fail") {
            cell2.className = "fail";
        }
    });
}

function populateSpecConformity(specConformity) {
    if (!specConformity || !specConformity.length) return;

    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    specTable.innerHTML = '';

    specConformity.forEach(item => {
        const row = specTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        const cell3 = row.insertCell(2);
        const cell4 = row.insertCell(3);

        cell1.textContent = item.parameter;
        cell2.textContent = item.expected;
        cell3.textContent = item.got;
        cell4.textContent = item.result;
        if (item.result === "Pass") {
            cell4.className = "pass";
        } else if (item.result === "Fail") {
            cell4.className = "fail";
        }
    });
}

function populateFailureDetails(failureDetails) {
    if (!failureDetails || !failureDetails.length) {
        const failureList = document.getElementById('failure-list');
        failureList.innerHTML = '';
        
        const li = document.createElement('li');
        li.textContent = '-';
        failureList.appendChild(li);
        return;
    }

    const failureList = document.getElementById('failure-list');
    failureList.innerHTML = '';

    failureDetails.forEach(detail => {
        const li = document.createElement('li');
        li.textContent = detail;
        failureList.appendChild(li);
    });
}

function populateDefaultValues() {
    const deviceSpeed = document.getElementById('device-speed').value;
    const speedValue = parseInt(deviceSpeed.split(' ')[0]); //get only the number
    
    let defaultValues;
    
    //a swetch case performs better than a for loop
    switch(speedValue) {
        case 4:
            defaultValues = specDefaultValues[0];
            break;
        case 20:
            defaultValues = specDefaultValues[1];
            break;
        case 32:
            defaultValues = specDefaultValues[2];
            break;
        case 64:
            defaultValues = specDefaultValues[3];
            break;
        default:
            return;
    }
    
    const specConformityData = [];
    
    for (let i = 0; i < specParameterNames.length; i++) {
        const parameter = specParameterNames[i];
        let expected = defaultValues[i];
        
        if (i === 0) {
            expected = `${expected} MHz`;
        } else if (i < 7) {
            expected = `${expected} μs`;
        }
        
        specConformityData.push({
            parameter: parameter,
            expected: expected,
            got: "-",
            result: "-"
        });
    }
    
    populateSpecConformity(specConformityData);
    
    document.querySelector('.failure-details').style.display = 'none';
}

function initializeTablesWithPlaceholders() {
    const electricalParameters = [
        "Bus Max Speed", "VIH (High-level input voltage)", "VIL (Low-level input voltage)",
        "Ileak (Leakage current)", "Rise Time", "Falling Time", "Cycle Time"
    ];

    const electricalTable = document.getElementById('electrical-info-table').getElementsByTagName('tbody')[0];
    electricalTable.innerHTML = '';

    electricalParameters.forEach(param => {
        const row = electricalTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.textContent = param;
        cell2.textContent = '-';
    });

    const transmissionParameters = [
        "Transmission Type", "Valid Header", "Valid Source ID", "Valid Padding", "Valid CRC"
    ];

    const transmissionTable = document.getElementById('transmission-info-table').getElementsByTagName('tbody')[0];
    transmissionTable.innerHTML = '';

    transmissionParameters.forEach(param => {
        const row = transmissionTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.textContent = param;
        cell2.textContent = '-';
    });

    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    specTable.innerHTML = '';

    specParameterNames.forEach(param => {
        const row = specTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        const cell3 = row.insertCell(2);
        const cell4 = row.insertCell(3);

        cell1.textContent = param;
        cell2.textContent = '-';
        cell3.textContent = '-';
        cell4.textContent = '-';
    });

    document.querySelector('.failure-details').style.display = 'none';
}

function showNotification(message, type) {
    const notification = document.getElementById('notification');
    notification.textContent = message;
    notification.className = `notification ${type}`;
    notification.style.display = 'block';

    setTimeout(() => {
        notification.style.display = 'none';
    }, 3000);
}

document.addEventListener('DOMContentLoaded', function() {
    initializeTablesWithPlaceholders();
});

export { fetchReportData, populateDefaultValues };