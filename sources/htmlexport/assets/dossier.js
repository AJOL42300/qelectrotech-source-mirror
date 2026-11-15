/* Script of the QElectroTech technical dossier. No network, no module: the
   pages are opened from a folder, and the data are .js files loaded with
   <script src> because fetch() is refused on file://. */
(function () {
"use strict";

var I18N = window.DOSSIER_I18N || {};
function t(key) { return I18N[key] || key; }
function el(tag, attrs, text) {
	var node = document.createElement(tag);
	for (var k in (attrs || {}))
		node.setAttribute(k, attrs[k]);
	if (text !== undefined && text !== null)
		node.textContent = text;
	return node;
}
function svgEl(tag, attrs) {
	var node = document.createElementNS("http://www.w3.org/2000/svg", tag);
	for (var k in (attrs || {}))
		node.setAttribute(k, attrs[k]);
	return node;
}
function words(text) {
	return (text || "").toLowerCase().split(/\s+/).filter(Boolean);
}
function matches(haystack, ws) {
	for (var i = 0; i < ws.length; ++i)
		if (haystack.indexOf(ws[i]) < 0)
			return false;
	return true;
}
function param(name) {
	var m = new RegExp("[?&]" + name + "=([^&]*)").exec(location.search);
	return m ? decodeURIComponent(m[1].replace(/\+/g, " ")) : "";
}
function folioFile(n) {
	var f = (window.DOSSIER_FILES || {})[n];
	return f || ("folio-" + n + ".html");
}

/* ---- sortable, filterable tables ---- */
function initTable() {
	var table = document.querySelector("table.data");
	if (!table)
		return;
	var input = document.getElementById("filter");
	var count = document.getElementById("count");
	var rows = Array.prototype.slice.call(table.tBodies[0].rows);
	var texts = rows.map(function (r) { return r.textContent.toLowerCase(); });
	function apply() {
		var ws = words(input ? input.value : "");
		var shown = 0;
		rows.forEach(function (r, i) {
			var ok = matches(texts[i], ws);
			r.style.display = ok ? "" : "none";
			if (ok) ++shown;
		});
		if (count)
			count.textContent = shown + " / " + rows.length;
	}
	if (input) {
		input.addEventListener("input", apply);
		var initial = param("q");
		if (initial) input.value = initial;
	}
	Array.prototype.forEach.call(table.tHead.rows[0].cells, function (th, col) {
		var asc = true;
		th.addEventListener("click", function () {
			var numeric = rows.every(function (r) {
				var v = r.cells[col].textContent.trim();
				return v === "" || !isNaN(parseFloat(v.replace(",", ".")));
			});
			var sorted = rows.slice().sort(function (a, b) {
				var x = a.cells[col].textContent.trim();
				var y = b.cells[col].textContent.trim();
				var r;
				if (numeric)
					r = (parseFloat(x.replace(",", ".")) || 0)
						- (parseFloat(y.replace(",", ".")) || 0);
				else
					r = x.localeCompare(y, undefined, {numeric: true});
				return asc ? r : -r;
			});
			asc = !asc;
			sorted.forEach(function (r) { table.tBodies[0].appendChild(r); });
			rows = sorted;
			texts = rows.map(function (r) { return r.textContent.toLowerCase(); });
		});
	});
	apply();
	if (location.hash.length > 1) {
		var target = document.getElementById(location.hash.slice(1));
		if (target) target.scrollIntoView({block: "center"});
	}
}

/* ---- global search ---- */
function initSearch() {
	var input = document.getElementById("filter");
	var out = document.getElementById("results");
	var count = document.getElementById("count");
	var index = window.DOSSIER_SEARCH || [];
	var lower = index.map(function (e) { return (e[0] + " " + e[1]).toLowerCase(); });
	function run() {
		var ws = words(input.value);
		out.textContent = "";
		if (!ws.length) {
			count.textContent = "";
			return;
		}
		var shown = 0, limit = 500;
		for (var i = 0; i < index.length; ++i) {
			if (!matches(lower[i], ws))
				continue;
			if (++shown > limit)
				continue;
			var div = el("div", {"class": "res"});
			div.appendChild(el("span", {"class": "kind"}, index[i][0]));
			var a = el("a", {href: index[i][2]}, index[i][1]);
			div.appendChild(a);
			if (index[i][3])
				div.appendChild(el("span", {"class": "muted"}, "  " + index[i][3]));
			out.appendChild(div);
		}
		count.textContent = shown > limit
			? limit + " / " + shown : String(shown);
	}
	input.addEventListener("input", run);
	input.value = param("q");
	run();
}

/* ---- home page ---- */
function initHome() {
	var data = window.DOSSIER_INDEX || {functions: [], folios: []};
	var list = document.getElementById("fnlist");
	var frame = document.getElementById("overview");
	var base = frame ? frame.getAttribute("data-src") : "";
	var items = {};
	function show(anchor) {
		for (var k in items)
			items[k].className = k === anchor ? "on" : "";
		if (items[anchor] && items[anchor].scrollIntoView)
			items[anchor].scrollIntoView({block: "nearest"});
	}
	data.functions.forEach(function (f) {
		var li = el("li");
		var head = el("div");
		if (frame) {
			var a = el("a", {href: "#" + f.anchor, "class": "tag"},
				f.id + " - " + f.tag);
			a.addEventListener("click", function (ev) {
				ev.preventDefault();
				frame.src = base + "#" + f.anchor;
				show(f.anchor);
			});
			head.appendChild(a);
		} else {
			head.appendChild(el("span", {"class": "tag"}, f.id + " - " + f.tag));
		}
		head.appendChild(el("span", {"class": "muted"}, "  " + f.title));
		li.appendChild(head);
		var folios = el("div", {"class": "folios"});
		if (!f.folios.length)
			folios.appendChild(el("span", {"class": "muted"}, t("No folio")));
		f.folios.forEach(function (n) {
			folios.appendChild(el("a", {href: folioFile(n)}, String(n)));
		});
		li.appendChild(folios);
		list.appendChild(li);
		items[f.anchor] = li;
	});
	window.addEventListener("message", function (ev) {
		var d = ev.data;
		if (d && d.dossier === "view")
			show(d.anchor);
	});
	var toc = document.getElementById("toc");
	data.folios.forEach(function (f) {
		toc.appendChild(el("a", {href: folioFile(f.n)}, f.n + " - " + f.title));
	});
	var q = document.getElementById("tocfilter");
	if (q)
		q.addEventListener("input", function () {
			var ws = words(q.value);
			Array.prototype.forEach.call(toc.children, function (a) {
				a.style.display = matches(a.textContent.toLowerCase(), ws)
					? "" : "none";
			});
		});
}

/* ---- folio page ---- */
function initFolio() {
	var F = window.DOSSIER_FOLIO;
	if (!F)
		return;
	var scroll = document.getElementById("scroll");
	var viewer = document.getElementById("viewer");
	var img = document.getElementById("pic");
	var details = document.getElementById("details");
	var tip = document.getElementById("tip");
	var zoom = 1;
	var overlay = svgEl("svg", {id: "overlay", viewBox: "0 0 " + F.w + " " + F.h});
	viewer.appendChild(overlay);
	var elementNodes = {};

	function setZoom(z) {
		zoom = Math.max(0.1, Math.min(8, z));
		viewer.style.width = Math.round(F.w * zoom) + "px";
		viewer.style.height = Math.round(F.h * zoom) + "px";
	}
	function fit() { setZoom((scroll.clientWidth - 4) / F.w); }
	function addRect(cls, x, y, w, h, key, i, j) {
		var r = svgEl("rect", {"class": "h " + cls, x: x, y: y, width: w,
			height: h});
		r.setAttribute("data-k", key);
		r.setAttribute("data-i", i);
		if (j !== undefined) r.setAttribute("data-j", j);
		overlay.appendChild(r);
		return r;
	}

	F.elements.forEach(function (e, i) {
		elementNodes[e.uuid] = addRect("he", e.x, e.y, e.w, e.h, "e", i);
	});
	F.conductors.forEach(function (c, i) {
		var p = svgEl("path", {"class": "hc", d: c.d});
		p.setAttribute("data-k", "c");
		p.setAttribute("data-i", i);
		overlay.appendChild(p);
	});
	F.terminals.forEach(function (tm, i) {
		addRect("ht", tm.x - 4, tm.y - 4, 8, 8, "t", i);
	});
	F.links.forEach(function (l, i) {
		var a = svgEl("a", {href: l.href});
		var r = svgEl("rect", {"class": "h hl", x: l.x, y: l.y, width: l.w,
			height: l.h});
		r.setAttribute("data-k", "l");
		r.setAttribute("data-i", i);
		a.appendChild(r);
		overlay.appendChild(a);
	});

	function lines(rows) {
		return rows.filter(function (r) { return r[1]; });
	}
	function elementTitle(e) { return e.label || e.name || t("Element"); }
	function describe(node) {
		var k = node.getAttribute("data-k");
		var i = +node.getAttribute("data-i");
		if (k === "e") {
			var e = F.elements[i];
			var rows = [[t("Name"), e.name]];
			["function", "designation", "manufacturer_reference"]
				.forEach(function (key) {
					rows.push([t(key), e.info[key]]);
				});
			e.io.slice(0, 8).forEach(function (io) {
				rows.push([t("I/O"), io.address + " (" + io.type + ")"
					+ (io.entry ? " " + io.entry : "")
					+ (io.function ? " - " + io.function : "")]);
			});
			return {title: elementTitle(e), rows: lines(rows)};
		}
		if (k === "t") {
			var tm = F.terminals[i];
			var te = F.elements[tm.e];
			var rows2 = [
				[t("Terminal"), tm.name],
				[t("Wire"), tm.wire],
				[t("Connected to"), tm.to],
				[t("Function"), tm.function],
				[t("Colour"), tm.colour],
				[t("Section"), tm.section]
			];
			(tm.io || []).forEach(function (io) {
				rows2.push([t("I/O"), io.address + " (" + io.type + ")"
					+ (io.function ? " - " + io.function : "")]);
			});
			return {title: (te ? elementTitle(te) : "") + " : "
				+ (tm.name || "?"), rows: lines(rows2)};
		}
		if (k === "c") {
			var c = F.conductors[i];
			return {title: t("Wire") + " " + (c.wire || ""), rows: lines([
				[t("From"), c.from], [t("To"), c.to],
				[t("Function"), c.function], [t("Colour"), c.colour],
				[t("Section"), c.section], [t("Cable"), c.cable]
			])};
		}
		var l = F.links[i];
		return {title: l.tip || t("Go to"), rows: []};
	}
	overlay.addEventListener("mousemove", function (ev) {
		var node = ev.target;
		if (!node.getAttribute || !node.getAttribute("data-k")) {
			tip.style.display = "none";
			return;
		}
		var d = describe(node);
		tip.textContent = "";
		tip.appendChild(el("div", null, d.title));
		d.rows.forEach(function (r) {
			tip.appendChild(el("div", null, r[0] + ": " + r[1]));
		});
		tip.style.display = "block";
		tip.style.left = Math.min(ev.clientX + 14, window.innerWidth - 300) + "px";
		tip.style.top = (ev.clientY + 16) + "px";
	});
	overlay.addEventListener("mouseleave", function () {
		tip.style.display = "none";
	});

	var selected = null;
	function select(uuid, scrollTo) {
		if (selected) selected.setAttribute("class", "h he");
		selected = elementNodes[uuid] || null;
		details.textContent = "";
		if (!selected) {
			details.appendChild(el("p", {"class": "muted"},
				t("Click an element to see its details.")));
			return;
		}
		selected.setAttribute("class", "h he sel");
		var e = F.elements[+selected.getAttribute("data-i")];
		details.appendChild(el("h3", null, elementTitle(e)));
		var table = el("table");
		var rows = [[t("Name"), e.name]];
		Object.keys(e.info).sort().forEach(function (key) {
			rows.push([t(key), e.info[key]]);
		});
		rows.forEach(function (r) {
			if (!r[1]) return;
			var tr = el("tr");
			tr.appendChild(el("td", null, r[0]));
			tr.appendChild(el("td", null, r[1]));
			table.appendChild(tr);
		});
		details.appendChild(table);
		function linkLine(label, href, text) {
			var p = el("p");
			p.appendChild(document.createTextNode(label + " "));
			p.appendChild(el("a", {href: href}, text));
			details.appendChild(p);
		}
		if (e.bom >= 0)
			linkLine(t("Bill of materials:"), "nomenclature.html#b" + e.bom,
				t("line") + " " + (e.bom + 1));
		linkLine(t("Devices list:"), "equipment.html#d-" + e.uuid, e.label || e.name);
		e.io.forEach(function (io) {
			linkLine(t("I/O"), "io.html?q=" + encodeURIComponent(io.address),
				io.address + " " + (io.function || ""));
		});
		e.xref.forEach(function (x) {
			linkLine(t("Cross reference:"), folioFile(x.folio) + "#e=" + x.uuid,
				x.folio + " - " + x.label + (x.position ? " (" + x.position + ")" : ""));
		});
		var rect = selected.getBoundingClientRect();
		if (scrollTo) {
			scroll.scrollLeft = Math.max(0, e.x * zoom - scroll.clientWidth / 2
				+ e.w * zoom / 2);
			scroll.scrollTop = Math.max(0, e.y * zoom - scroll.clientHeight / 2
				+ e.h * zoom / 2);
		}
		return rect;
	}
	overlay.addEventListener("click", function (ev) {
		var node = ev.target;
		var k = node.getAttribute && node.getAttribute("data-k");
		if (k === "e") {
			var e = F.elements[+node.getAttribute("data-i")];
			location.hash = "e=" + e.uuid;
		} else if (k === "t") {
			var tm = F.terminals[+node.getAttribute("data-i")];
			var te = F.elements[tm.e];
			if (te) location.hash = "e=" + te.uuid;
		}
	});
	function fromHash() {
		var m = /e=([^&]+)/.exec(location.hash);
		select(m ? decodeURIComponent(m[1]) : "", true);
	}
	window.addEventListener("hashchange", fromHash);

	document.getElementById("zin").addEventListener("click", function () { setZoom(zoom * 1.25); });
	document.getElementById("zout").addEventListener("click", function () { setZoom(zoom / 1.25); });
	document.getElementById("zfit").addEventListener("click", fit);
	document.getElementById("z100").addEventListener("click", function () { setZoom(1); });
	scroll.addEventListener("wheel", function (ev) {
		if (!ev.ctrlKey) return;
		ev.preventDefault();
		setZoom(zoom * (ev.deltaY < 0 ? 1.15 : 1 / 1.15));
	}, {passive: false});
	document.addEventListener("keydown", function (ev) {
		if (ev.target && /input|textarea/i.test(ev.target.tagName)) return;
		if (ev.key === "ArrowLeft" && F.prev) location.href = folioFile(F.prev);
		if (ev.key === "ArrowRight" && F.next) location.href = folioFile(F.next);
	});

	fit();
	fromHash();
}

window.Dossier = {
	boot: function () {
		var page = document.body.getAttribute("data-page");
		if (page === "home") initHome();
		else if (page === "folio") initFolio();
		else if (page === "search") initSearch();
		else if (page === "table") initTable();
	}
};
document.addEventListener("DOMContentLoaded", window.Dossier.boot);
})();
