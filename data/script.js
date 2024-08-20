function openRelay(){
    var pulseValue = document.getElementById("pulseValue").value;
    var openRequest = new XMLHttpRequest;
    openRequest.open("GET", "/open?duration=" + pulseValue, true);
    openRequest.send();
}

function closeRelay(){
    var pulseValue = document.getElementById("pulseValue").value;
    var closeRequest = new XMLHttpRequest;
    closeRequest.open("GET", "/close?duration=" + pulseValue, true);
    closeRequest.send();
}

function openRelayTest(){
    var openTestRequest = new XMLHttpRequest;
    openTestRequest.open("GET", "/openrelaytest");
    openTestRequest.onreadystatechange = function(){
        if(openTestRequest.readyState == 4 && openTestRequest.status == 200){
            document.getElementById("timeD13").innerHTML = "Czas otwarcia (D13): " + openTestRequest.responseText;
        }
    }
    openTestRequest.send();
}

function closeRelayTest(){
    var closeTestRequest = new XMLHttpRequest;
    closeTestRequest.open("GET", "/closerelaytest");
    closeTestRequest.onreadystatechange = function(){
        if(closeTestRequest.readyState == 4 && closeTestRequest.status == 200){
            document.getElementById("timeD12").innerHTML = "Czas zamknięcia (D12): " + closeTestRequest.responseText;
        }
    }
    closeTestRequest.send();
}

function updatePinStateImage(){
    fetch('/pinState')
        .then(response => response.text())
        .then(state => {
            const img = document.getElementById('pinStateImage');
            if(state == '1'){
                img.src = "closed.png";
            }
            else{
                img.src = "opened.png";
            }
        })
        .catch(error => console.error("Error fetching pin state", error));
}
setInterval(updatePinStateImage, 1000);
updatePinStateImage();