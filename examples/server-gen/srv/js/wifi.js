function serialize(form) {
	var field, l, s = [];
	if (typeof form == 'object' && form.nodeName == "FORM") {
		var len = form.elements.length;
		for (var i=0; i<len; i++) {
			field = form.elements[i];
			if (field.name && !field.disabled && field.type != 'file' && field.type != 'reset' && field.type != 'submit' && field.type != 'button') {
				if (field.type == 'select-multiple') {
					l = form.elements[i].options.length;
					for (var j=0; j<l; j++) {
						if(field.options[j].selected)
							s[s.length] = encodeURIComponent(field.name) + "=" + encodeURIComponent(field.options[j].value);
					}
				} else if ((field.type != 'checkbox' && field.type != 'radio') || field.checked) {
					s[s.length] = encodeURIComponent(field.name) + "=" + encodeURIComponent(field.value);
				}
			}
		}
	}
	return s.join('&').replace(/%20/g, '+');
}


function get_wifi_icon(x,y,rssi,auth){
	var col_1 = '#000000';
	var col_0 = '#DDDDDD';

	var icon = '<svg viewBox="0 -20 100 100" width="'+x+'" height="'+y+'">';
	icon += '<g><circle style="fill:'+col_1+';fill-opacity:1;stroke:'+col_1+';stroke-width:1.5"cx="18.5"cy="72"r="7"/>';

	icon += '<path style="fill:none;stroke:'+((rssi > -80)? col_1 : col_0 )+';stroke-width:6"'+
	'sodipodi:type="arc"' +
	'sodipodi:cx="15"sodipodi:cy="75"'+
	'sodipodi:rx="20"sodipodi:ry="20"'+
	'sodipodi:start="4.7"sodipodi:end="0"'+
	'd="M 14.75,55.00 A 20,20 0 0 1 29.05,60.77 20,20 0 0 1 35,75"'+
	'sodipodi:open="true" />';


	icon += '<path style="fill:none;stroke:'+((rssi > -70)? col_1 : col_0 )+';stroke-width:6"'+
	'sodipodi:type="arc"' +
	'sodipodi:cx="15" sodipodi:cy="75"' +
	'sodipodi:rx="30" sodipodi:ry="30"' +
	'sodipodi:start="4.7" sodipodi:end="0"' +
	'd="M 14.63,45.00 A 30,30 0 0 1 36.0,53.66 30,30 0 0 1 45,75"'+
	'sodipodi:open="true" />';


	icon += '<path style="fill:none;stroke:'+((rssi > -67)? col_1 : col_0 )+';stroke-width:6"'+
  'sodipodi:type="arc"'+
	'sodipodi:cx="15" sodipodi:cy="75"'+
	'sodipodi:rx="40" sodipodi:ry="40"'+
	'sodipodi:start="4.7" sodipodi:end="0"'+
	'd="M 14.504453,35.00307 A 40,40 0 0 1 43.108523,46.541066 40,40 0 0 1 55,75"'+
	'sodipodi:open="true"/>';

	if(auth !== "OPEN"){
		icon += '<rect style="fill:'+col_1+';fill-opacity:1;stroke:'+col_1+';stroke-width:1"width="20" height="14" x="51" y="32"/>';
		icon += '<ellipse style="fill:none;fill-opacity:1;stroke:'+col_1+';stroke-width:3"cx="61"cy="31"rx="7.5"ry="9"/>';
	}
	icon += '</g></svg>';
	return icon;
}

function get_wifi_AP_info(esp_response){
	var js = JSON.parse(esp_response);
	var info = '<small>';
	info+='AP:<br>';
	info+='address:'+js.ap.ip_info.ip+'<br>';
	info+='netmask:'+js.ap.ip_info.netmask+'<br>';
	info+='gateway:'+js.ap.ip_info.gw+'<br>';
	info+='</small>';
	return info;
}

function get_wifi_STA_info(esp_response){
	var js = JSON.parse(esp_response);
	var info = '<small>';
	info+='STA:<br>';
	info+='status:'+js.sta.status+'<br>';
	if(js.sta.status === "connected"){
		info+='SSID:'+js.sta.connection.ssid+'<br>';
		info+='RSSI:'+js.sta.connection.rssi+'<br>';
		info+='AUTH:'+js.sta.connection.authmode+'<br>';
	}
	info+='<br>';
	info+='address:'+js.sta.ip_info.ip+'<br>';
	info+='netmask:'+js.sta.ip_info.netmask+'<br>';
	info+='gateway:'+js.sta.ip_info.gw+'<br>';
	info+='</small>';
	return info;
}

function update_wifi_info(ap_info,sta_info) {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if(this.readyState == 4 && this.status == 200){
			ap_info.innerHTML = get_wifi_AP_info(this.responseText);
			sta_info.innerHTML = get_wifi_STA_info(this.responseText);
		}
	};
	xhttp.open("GET","/info", true);
	xhttp.send();
}
