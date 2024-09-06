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
    var pulseValue = document.getElementById("pulseValue").value;
    var openTestRequest = new XMLHttpRequest;
    openTestRequest.open("GET", "/openrelaytest?duration=" + pulseValue, true);
    openTestRequest.onreadystatechange = function(){
        if(openTestRequest.readyState == 4 && openTestRequest.status == 200){
            document.getElementById("openTime").innerHTML = "Czas otwarcia: " + openTestRequest.responseText;
        }
    }
    openTestRequest.send();
}
function closeRelayTest(){
    var pulseValue = document.getElementById("pulseValue").value;
    var closeTestRequest = new XMLHttpRequest;
    closeTestRequest.open("GET", "/closerelaytest?duration=" + pulseValue, true);
    closeTestRequest.onreadystatechange = function(){
        if(closeTestRequest.readyState == 4 && closeTestRequest.status == 200){
            document.getElementById("closeTime").innerHTML = "Czas zamknięcia: " + closeTestRequest.responseText;
        }
    }
    closeTestRequest.send();
}
function localCloseTime(){
    var localCloseRequest = new XMLHttpRequest;
    localCloseRequest.open("GET", "/localclosetime");
    localCloseRequest.onreadystatechange = function(){
        if(localCloseRequest.readyState == 4 && localCloseRequest.status == 200){
            document.getElementById("localRelay").innerHTML = "Czas własny przekaźnika: " + localCloseRequest.responseText;
        }
    }
    localCloseRequest.send();
}
function localOpenTime(){
    var localOpenRequest = new XMLHttpRequest;
    localOpenRequest.open("GET", "/localopentime");
    localOpenRequest.onreadystatechange = function(){
        if(localOpenRequest.readyState = 4 && localOpenRequest.status == 200){
            document.getElementById("localRelay").innerHTML = "Czas własny przekaźnika: " + localOpenRequest.responseText;
        }
    }
    localOpenRequest.send();
}
function doBothOpen(){
    localOpenTime();
    openRelayTest();
}
function doBothClose(){
    localCloseTime();
    closeRelayTest();
}
function updatePinStateImage(){
    fetch('/pinState')
        .then(response => response.text())
        .then(state => {
            const img = document.getElementById('pinStateImage');
            if(state == '1'){
                img.src = "closed.png";
            }
            else if(state == '0'){
                img.src = "opened.png";
            }
            else{
                img.src = "unknown.jpg";
            }
        })
        .catch(error => console.error("Error fetching pin state", error));
}

setInterval(updatePinStateImage, 1000);
updatePinStateImage();