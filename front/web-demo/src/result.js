// report.js - Handles fetching and displaying DCP validation report data

// Function to fetch data from the backend
async function fetchReportData() {

    // Get selected values
    const deviceType = document.getElementById('device-type').value;
    const deviceSpeed = document.getElementById('device-speed').value;
    
    // Validate selections
    if (deviceType === "Selecione" || deviceSpeed === "Selecione") {
        showNotification('Please select both Device Type and Device Speed', 'error');
        return;
    }

    try {
        // Show loading indicator
        document.getElementById('loading-indicator').style.display = 'block';
        
        // Prepare request body
        const requestBody = {
            deviceType: deviceType,
            deviceSpeed: deviceSpeed
        };

        // Fetch data from the backend API
        const response = await fetch('http:/dcp-validator/api/v1/validation', {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(requestBody)
        });
        
        // Check if the request was successful
        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }
        
        // Parse the JSON response
        const data = await response.json();
        
        // Populate the UI with the fetched data
        populateDeviceInfo(data.deviceInfo);
        populateElectricalInfo(data.electricalInfo);
        populateSpecConformity(data.specConformity);
        populateFailureDetails(data.failureDetails);
        
        // Hide loading indicator after data is loaded
        document.getElementById('loading-indicator').style.display = 'none';
        
        // Show success message
        showNotification('Data loaded successfully', 'success');
    } catch (error) {
        console.error('Error fetching report data:', error);
        
        // Hide loading indicator in case of error
        document.getElementById('loading-indicator').style.display = 'none';
        
        // Show error message
        showNotification('Failed to load data from server', 'error');
    }
}

// Function to populate device information
function populateDeviceInfo(deviceInfo) {
    if (!deviceInfo) return;
    
    document.getElementById('device-name').value = deviceInfo.name || '';
    document.getElementById('device-version').value = deviceInfo.version || '';
    document.getElementById('device-type').value = deviceInfo.type || '';
    document.getElementById('device-speed').value = deviceInfo.speed || '';
}

// Function to populate electrical information table
function populateElectricalInfo(electricalInfo) {
    if (!electricalInfo || !electricalInfo.length) return;
    
    const electricalTable = document.getElementById('electrical-info-table').getElementsByTagName('tbody')[0];
    
    // Clear existing table rows
    electricalTable.innerHTML = '';
    
    // Add new rows from the data
    electricalInfo.forEach(item => {
        const row = electricalTable.insertRow();
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        
        cell1.textContent = item.parameter;
        cell2.textContent = item.value;
    });
}

// Function to populate specification conformity table
function populateSpecConformity(specConformity) {
    if (!specConformity || !specConformity.length) return;
    
    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    
    // Clear existing table rows
    specTable.innerHTML = '';
    
    // Add new rows from the data
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
        cell4.className = item.result === "Pass" ? "pass" : "fail";
    });
}

// Function to populate failure details list
function populateFailureDetails(failureDetails) {
    if (!failureDetails || !failureDetails.length) return;
    
    const failureList = document.getElementById('failure-list');
    
    // Clear existing list items
    failureList.innerHTML = '';
    
    // Add new list items from the data
    failureDetails.forEach(detail => {
        const li = document.createElement('li');
        li.textContent = detail;
        failureList.appendChild(li);
    });
}

// Function to show notification messages
function showNotification(message, type) {
    const notification = document.getElementById('notification');
    notification.textContent = message;
    notification.className = `notification ${type}`;
    notification.style.display = 'block';
    
    // Hide notification after 3 seconds
    setTimeout(() => {
        notification.style.display = 'none';
    }, 3000);
}

// Initialize tables with placeholder dashes
function initializeTablesWithPlaceholders() {
    // Initialize electrical information table with placeholders
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

    // Initialize specification conformity table with placeholders
    const specParameters = [
        "Speed Test", "High Time", "Low Time", "Sync burst", "Bit Sync", "Bus Yield"
    ];

    const specTable = document.getElementById('spec-conformity-table').getElementsByTagName('tbody')[0];
    specTable.innerHTML = '';

    specParameters.forEach(param => {
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

    // Initialize failure details with placeholder
    const failureList = document.getElementById('failure-list');
    failureList.innerHTML = '';

    const li = document.createElement('li');
    li.textContent = '-';
    failureList.appendChild(li);
}

const defaultValues = [
    []
    []
    []
    []
]


function populateDefaultValues(){

}

// Initialize with mock data when page loads
document.addEventListener('DOMContentLoaded', function() {
    loadMockData();
});

export {fetchReportData};
