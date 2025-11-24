function tab(e, id) {
  let btn = document.getElementById("menu-btn");
  let tab = document.getElementsByClassName("tab");
  for (var i = 0; i < tab.length; i++)
    tab[i].style.display = "none";

  document.getElementById(id).style.display = "block";
  e.currentTarget.className += " active";
  btn.checked = false;
}

function defaults(e) {
    if (confirm("Do you want to revert default settings?") == true) {
	    fetch(esp_url+"/settings?action=erase");
		getSettingsForm();
	    alert("Board settings reset to defaults");
    }
}

function reset(e) {
    if (confirm("Do you want to reset the board?") == true) {
	    fetch(esp_url+"/settings?action=restart");
	    alert("Board reset - disconnected");
    }
}
function getSettingsForm(){
	const tzlist = [
		"Europe/London",
		"Europe/Dublin",
		"Europe/Lisbon",
		"Europe/Paris",
		"Europe/Brussels",
		"Europe/Berlin",
		"Europe/Madrid",
		"Europe/Rome",
		"Europe/Amsterdam",
		"Europe/Stockholm",
		"Europe/Zurich",
		"Europe/Athens",
		"Europe/Kyiv",
		"Europe/Warsaw",
		"Europe/Istanbul",
		"Europe/Moscow"
	];

	fetch(esp_url+"/settings")
	.then(r => r.json())
	.then(js => {
		const div = document.getElementById('brd');

		let html = `<form id="brd-form" method="post">`;
		js.data.groups.forEach((gr, i) => {
			html+=`${gr.label}:<br>`;
			html+='<small>';
			gr.settings.forEach((item, i) => {
				switch (item.type) {
					case "BOOL":
						html+=`<input type="checkbox" name="${gr.id}:${item.id}" ${item.val?"checked":''}>${item.label}<br>`;
						break;
					case "NUM":
						html+=`<span class="label-inline">${item.label}</span><input type="number" name="${gr.id}:${item.id}" min=${item.min} max=${item.max} value=${item.val} style="width:250px"><br>`;
						break;
					case "ONEOF":
						html+=`<span class="label-inline">${item.label}</span><select name="${gr.id}:${item.id}" style="width:160px">`;
						item.options.forEach((txt, i) => { html+=` <option value=${i} ${item.val==i?"selected":''}>${txt}</option>`;});
						html+=`</select><br>`;
						break;
					case "TEXT":
						html+=`<span class="label-inline">${item.label}</span><input type="text" name="${gr.id}:${item.id}" value="${item.val}" maxlength="${item.len}" style="width:250px"><br>`;
						break;
					case "TIME":
						let time = String(item.hh).padStart(2,'0')+":"+String(item.mm).padStart(2,'0');
						html+=`<span class="label-inline">${item.label}</span><input type="time" name="${gr.id}:${item.id}" min="00:00" max="23:59" value="${time}" style="width:250px"/><br>`;
						break;
                      case "DATE":
                        let date = String(item.year).padStart(4,'0')+"-"+String(item.month).padStart(2,'0')+"-"+String(item.day).padStart(2,'0');
                        html+=`<span class="label-inline">${item.label}</span><input type="date" name="${gr.id}:${item.id}"  value="${date}" style="width:250px"/><br>`;
                        break;
                      case "DATETIME":
                        let datetime = String(item.year).padStart(4,'0')+"-"+String(item.month).padStart(2,'0')+"-"+String(item.day).padStart(2,'0');
                        datetime+="T"+String(item.hh).padStart(2,'0')+":"+String(item.mm).padStart(2,'0');
                        html+=`<span class="label-inline">${item.label}</span><input type="datetime-local" name="${gr.id}:${item.id}"  value="${datetime}" style="width:250px"/><br>`;
                        break;
                      case "TIMEZONE":
                        html+=`<span class="label-inline">${item.label}</span><select name="${gr.id}:${item.id}" style="width:250px">`;
                        tzlist.forEach((txt, i) => { html+=`<option value="${txt}" ${item.val==txt?"selected":''}>${txt}</option>`;});
                        html+=`</select><br>`;
                        break;
					case "COLOR":
						html+=`<span class="label-inline">${item.label}</span><input type="color" name="${gr.id}:${item.id}" value="${item.val}" style="width:50px"/><br>`;
						break;
					default:
						break;
				}
			});
			html+=`</small>`;
		});
		html+=`<input type="submit" value="submit">`;
		html+=`<input class="button-outline" type="button" value="DEFAULTS" onclick='defaults()'>`;
		html+=`</form>`;
		html+=`<button onclick='reset()'>RESTART</button>`;

		div.innerHTML = html;
		const form = document.getElementById('brd-form');
		form.onsubmit = function(e){
			e.preventDefault();
			let data = decodeURIComponent(new URLSearchParams(new FormData(this)));
			fetch(esp_url+"/settings?action=set",
			{
				method: "POST",
				body: data
			})
			.then(r => r.json())
			.then(js => { alert("board config changed"); });
		};
	})
}


