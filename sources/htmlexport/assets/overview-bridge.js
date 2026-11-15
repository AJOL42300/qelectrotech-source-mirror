/* Added by QElectroTech to the copy of the overview picture: tells the page
   which view is shown, so that it can list the folios of its function. */
(function () {
	function send(anchor) {
		try {
			parent.postMessage({dossier: "view", anchor: anchor}, "*");
		} catch (e) {}
	}
	function current() {
		return (location.hash || "").replace(/^#/, "");
	}
	window.addEventListener("hashchange", function () {
		send(current());
	});
	document.addEventListener("click", function (event) {
		var node = event.target;
		while (node && node.nodeType === 1 && node.localName !== "a")
			node = node.parentNode;
		if (!node || node.nodeType !== 1)
			return;
		var href = node.getAttribute("href")
			|| node.getAttributeNS("http://www.w3.org/1999/xlink", "href");
		if (href && href.charAt(0) === "#")
			send(href.slice(1));
	}, true);
	send(current());
})();
