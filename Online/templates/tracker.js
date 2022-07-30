/*function initMap(data) {
    
    var center = {lat: data[0][`lat`], data[0][`lon`]};
    var map = new google.maps.Map(document.getElementById('map'), {
        zoom: 4,
        center: center
    });
    
    var infowindow =  new google.maps.InfoWindow({});
    var marker;
    
    data.forEach(e => {
        marker = new google.maps.Marker({
        position: new google.maps.LatLng(e[`lat`], e[`lon`]),
        map: map,
    });
        google.maps.event.addListener(marker, 'click', (function (marker) {
            return function () {
                infowindow.setContent(`Latitude: ${e[`lat`]}<br>Longitude: ${e[`lon`]}`);
                infowindow.open(map, marker);
            }
        })(marker));
    });
}*/


  
const http = require('http')
const fs = require('fs')

const server = http.createServer((req, res) => {
  res.writeHead(200, { 'content-type': 'text/html' })
  fs.createReadStream('tracker.html').pipe(res)
})

server.listen(process.env.PORT || 3000)

