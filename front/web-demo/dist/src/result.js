// result.js - Handles fetching and displaying DCP validation report data

const specDelta = [20, 4, 2.5, 1.25];
const specParameterNames = [
    "Speed", "Bit High Time", "Bit Low Time", "Sync Time", "Bit Sync Time", "Bit Sync High", "Bit Sync Low", "Bus Yield"
];

const specDefaultValues = [
    // Speed (MHz), Bit High (μs), Bit Low (μs), Sync Time (μs), Bit Sync Time (μs), Bit Sync High (μs), Bit Sync Low (μs), Bus Yield
    [4,  2*specDelta[0], specDelta[0], 25*specDelta[0], 15*specDelta[0], 7.5*specDelta[0], 7.5*specDelta[0], "Yes"],
    [20, 2*specDelta[1], specDelta[1], 25*specDelta[1], 15*specDelta[1], 7.5*specDelta[1], 7.5*specDelta[1], "Yes"],
    [32, 2*specDelta[2], specDelta[2], 25*specDelta[2], 15*specDelta[2], 7.5*specDelta[2], 7.5*specDelta[2], "Yes"],
    [64, 2*specDelta[3], specDelta[3], 25*specDelta[3], 15*specDelta[3], 7.5*specDelta[3], 7.5*specDelta[3], "Yes"]
];

const mock = {
    electricalInfo: {
        "VIH (High-level input voltage)": 5.1,
        "VIL (Low-level input voltage)": 5.1,
        "Rise Time": 5.1,
        "Falling Time": 5.1,
        "Cycle Time": 5.1,
        "Bus Max Speed": 5.1,
    },
    transmissionInfo: {
        Type: 0,
        Sync: {status: true},
        BitSync: {status: true},
        Size: {status: true},
        L3: {status: true}
    },
    specConformity: {

       "Speed": 32,
       "Bit High Time": 5,
       "Bit Low Time": 2.5,
       "Sync Time": 62,
       "Bit Sync Time": 37.5,
       "Bit Sync High": 19,
       "Bit Sync Low": 19,
       "Bus Yield": true
    }
}

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
            isController: deviceType === 'controlador',
            deviceSpeed: parseInt(deviceSpeed.split(' ')[0])
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
    if (!electricalInfo) return;

    const electricalTable = document.getElementById('electrical-info-table').getElementsByTagName('tbody')[0];
    const rows = electricalTable.rows;

    for (let i = 0; i < rows.length; i++){
        let cells = rows[i].cells;

        const value = electricalInfo[cells[0].textContent];
        if (value !== undefined){
            cells[1].textContent = value;
        }
    }
}

function populateTransmissionInfo(transmissionInfo) {
    if (!transmissionInfo) return;

    const transmissionTable = document.getElementById('transmission-info-table').getElementsByTagName('tbody')[0];
    const rows = transmissionTable.rows;

    for (let i = 0; i < rows.length; i++){
        let cells = rows[i].cells;

        const value = transmissionInfo[cells[0].textContent];
        if (value === undefined) continue;

        cells[1].textContent = value.status ? "Pass": "Fail";
        cells[1].className = value.status ? "pass": "fail";

        if(!value){
            populateFailureDetails([value.reason]);
        }
    }

    const typeCells = rows[0].cells;    
    const transmissionType = transmissionInfo.Type == 0? "L3": "Generic";
    typeCells[1].textContent = transmissionType;
    typeCells[1].className = transmissionInfo[transmissionType]? "pass": "fail";

    /*
    if (transmissionInfo.type === 0){

        const L3 = ["Header", "Source ID", "Padding", "CRC"]

        if(transmissionInfo["L3"]){
            L3.forEach(h => {
                const row = transmissionTable.insertRow();
                row.insertCell(0).textContent = h;
                const cell = row.insertCell(1);

                cell.textContent = "Pass";
                cell.className = "pass";
            });
        }else{
            L3.forEach(h => {
                
                const value = transmissionInfo[h];
                if (value === undefined) return;

                const row = transmissionTable.insertRow();
                row.insertCell(0).textContent = h;
                const cell = row.insertCell(1);

                cell.textContent = value.status ? "Pass": "Fail";
                cell.className = value.status ? "pass": "fail";

                if(!value){
                    populateFailureDetails([value.reason]);
                }
            });
        }
    }
    */
}

function populateSpecConformity(specConformity) {
    if (!specConformity) return;

    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    const rows = specTable.rows;
    
    for (let i = 0; i < rows.length; i++){
        let cells = rows[i].cells;

        const value = specConformity[cells[0].textContent];
        if (value === undefined) continue;

        const splits = cells[1].textContent.split(' ');

        if (cells[0].textContent === "Bus Yield"){
            cells[2].textContent = value? "Yes": "No";
            
            if (cells[2].textContent === cells[1].textContent){
                cells[3].textContent = "Pass";
                cells[3].className = "pass";
            }else{
                cells[3].textContent = "Fail";
                cells[3].className = "fail";
            }

            return;
        }else {
            cells[2].textContent = "".concat(value, " ", splits[1]);

            const v = parseFloat(splits[0]);
        
            if (value >= v-v*.02 && value <= v+v*.02){
                cells[3].textContent = "Pass";
                cells[3].className = "pass";
            }else{
                cells[3].textContent = "Fail";
                cells[3].className = "fail";
            }
        }
    }
}

function populateFailureDetails(failureDetails) {
    if (!failureDetails || !failureDetails.length) {
        const failureList = document.getElementById('failure-list');
        failureList.innerHTML = '';
        return;
    }

    const failureList = document.getElementById('failure-list');

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
    
    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    specTable.innerHTML = '';

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
        
        const row = specTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        const cell3 = row.insertCell(2);
        const cell4 = row.insertCell(3);

        cell1.textContent = parameter;
        cell2.textContent = expected;
        cell3.textContent = "-";
        cell4.textContent = "-";
    }

    //populateSpecConformity(mock.specConformity);
    
    document.querySelector('.failure-details').style.display = 'none';
}

function initializeTablesWithPlaceholders() {
    const electricalParameters = [
        "VIH (High-level input voltage)", "VIL (Low-level input voltage)",
        "Rise Time", "Falling Time", "Cycle Time", "Bus Max Speed" 
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
        "Transmission Type", "Sync", "BitSync", "Size"
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

    //populateElectricalInfo(mock.electricalInfo);
    //populateTransmissionInfo(mock.transmissionInfo);
});

export { fetchReportData, populateDefaultValues };