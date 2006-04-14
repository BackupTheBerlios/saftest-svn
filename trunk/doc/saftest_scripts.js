function getSectionHeading(section)
{
    var ndx;
    for (ndx=0; ndx < section.childNodes.length; ndx++) {
        var node = section.childNodes[ndx];
        if ((node.tagName + "").toLowerCase() == "h2") {
            return node.firstChild.nodeValue + "";
        }
    }
    return "Invalid Section";
}

function generateTOC()
{
    var i, tocHead, tocDiv, contentHead, newListElement, 
        newElement, headers ;

    // Create our ul element for the TOC
    tocDiv = document.getElementById('toc');
    tocHead = document.createElement('ul');
    tocDiv.appendChild(tocHead);

    // Load all the sections
    contentHead = document.getElementById('content');
    //divs = contentHead.getElementsByTagName('div');
    divs = contentHead.childNodes
    string = "";
    var sectNum = 0;
    for(i = 0; i < divs.length; i++) {
        var tag = "" + divs[i].tagName;
        if (tag.toLowerCase() == 'div') {
            if (divs[i].getAttribute('class', 'section')) {
                var heading = getSectionHeading(divs[i]);
                sectNum++;

                // Add Anchor
                newElement = document.createElement('a');
                newElement.setAttribute('name', sectNum);
                divs[i].insertBefore(newElement, divs[i].firstChild);

                // Add TOC Entry
                newListElement = document.createElement('li');
                newLink = document.createElement('a');
                newLink.setAttribute('href', '#' + sectNum);
                newLink.appendChild(document.createTextNode((sectNum) + " " 
                                                            + heading));
                newListElement.appendChild(newLink);

                tocHead.appendChild(newListElement);
            }
        }
    }
}

function generateHeading() {
    if (!document.getElementById('heading')) {
        var ndx;
        // Create heading div section
        var headingSection = document.createElement('div');
        headingSection.setAttribute('id', 'heading');
        document.body.insertBefore(headingSection, document.body.firstChild);

        // Populate heading section with default fields
        newElement = document.createElement('h1');
        newElement.setAttribute('id', 'title');
        headingSection.appendChild(newElement);

        var parts = new Array("date", "author", "description", "toc");
        for(ndx = 0; ndx < parts.length; ndx++) {
            newElement = document.createElement('div');
            newElement.setAttribute('id', parts[ndx]);
            headingSection.appendChild(newElement);
        }

        var head = document.getElementsByTagName('head')[0];
        var metas = head.getElementsByTagName('meta');
        var targetElement;

        for(ndx = 0; ndx < metas.length; ndx++) {
            elem = metas[ndx];
            if (elem.getAttribute('name') == 'title') {
                targetElement = document.getElementById('title');
                targetElement.appendChild(document.createTextNode(
                                            elem.getAttribute('content')));
            }
            else if (elem.getAttribute('name') == 'author') {
                targetElement = document.getElementById('author');
                targetElement.appendChild(document.createTextNode(
                                            elem.getAttribute('content')));
            }
            else if (elem.getAttribute('name') == 'date') {
                targetElement = document.getElementById('date');
                targetElement.appendChild(document.createTextNode(
                     "Last Updated " + elem.getAttribute('content')));
            }
            else if (elem.getAttribute('name') == 'description') {
                var par = document.createElement('p');
                targetElement = document.getElementById('description');
                targetElement.appendChild(par);
                par.appendChild(document.createTextNode(
                                   elem.getAttribute('content')));
            }
            else if (elem.getAttribute('name') == 'copyright') {
                var par = document.createElement('p');
                par.setAttribute('id', 'copyright');
                par.appendChild(document.createTextNode(
                                   elem.getAttribute('content')));
                document.body.appendChild(par);
            }
        }

        var sourceElement = document.getElementsByTagName('title')[0];
        targetElement = document.getElementById('title');
        targetElement.appendChild(document.createTextNode(
                                  sourceElement.firstChild.nodeValue));
    }

    generateTOC();
}

/*
 * This is a recursive function for numbering sections.
 *  It expectes to find as childNodes to head, 0 or more divs with the 
 *  'section' style. The first header element in each section should be the
 *  title for the section.
 */
function numberHeaderLevel(head, tag, prepend)
{
    var headers = new Array();

    var sectNum = 0;
    var ndx;

    for (ndx = 0; ndx < head.childNodes.length; ndx++) {
        if (((head.childNodes[ndx].tagName + "").toLowerCase() == 'div') && 
           (head.childNodes[ndx].getAttribute('class') == 'section')) {
            headers.push(head.childNodes[ndx]);
        }
    }

    if (headers.length == 0) {
        return
    }

    headers.reverse();
    while (section = headers.pop()) {
        // Now that we have a section, we will go through each child and
        // find the header for the section
        elem = section.firstChild;
        sectNum++;
        while (elem) {
            var tagName = (elem.tagName + "").toLowerCase();
            if (tagName == tag) {
                var level = tag.substr(1,2);
                level++;
                var label = prepend + sectNum;
                var nextTag = (tag.substr(0,1) + level);
                elem.firstChild.nodeValue = label + " " + elem.firstChild.nodeValue;
                numberHeaderLevel(section, 
                                  nextTag,
                                  label + ".");
                break;
            }
            elem = elem.nextSibling;
        }
    }
}

function numberSections()
{
    var i, tocHead, tocDiv, contentHead, newListElement, 
        newElement, headers ;

    contentHead = document.getElementById('content');
    if (contentHead) {
        numberHeaderLevel(contentHead, 'h2', '');
    }
    else {
        return alert("Elements did not load");
    }

}
