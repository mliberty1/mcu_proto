// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

// Based upon http://electricimp.com/docs/gettingstarted/agents/
// https://community.electricimp.com/blog/how-to-serve-an-html-form-via-an-agent-and-deal-with-the-results/
// Log the URLs we need
server.log("Control URL: " + http.agenturl());


function getPage(message) {
    // The web page "template"
    local page = @"
    <!DOCTYPE html>
    <html>
    <head>
        <title>My Form</title>
    </head>
    <body>
        <h1>Electric IMP demo</h1>
        <form method='GET'>
            <input type='hidden' name='setMode' value='rotate_hue'>
            <input type='submit' value='Rotate Hue'></p>
        </form>
        <form method='GET'>
            <input type='hidden' name='setMode' value='off'>
            <input type='submit' value='Off'></p>
        </form>
        <p>" + message + @"</p>
    </body>
    </html>"
    
    return page    
}


function setMode(mode) {
    // mode string: The new device mode
    server.log("Mode changed to " + mode)
    device.send("setMode", mode); 
}


function requestHandler(request, response) {
    local mode = "off"
    local http_code = 200 // success
    local message = ""
    try {
        local method = request.method.toupper()
        if (method == "POST") {
            local data = http.urldecode(request.body)
            setMode(data.setMode)
            message = "OK (POST)"
        } else if (method == "GET") {
            if ("setMode" in request.query) {
                setMode(request.query.setMode); 
                message = "OK (GET)"
            } else if ("getMode" in request.query) {
                message = "Mode = " + mode
            } // else, just display page
        }
    } catch (ex) {
        message = "Internal Server Error: " + ex
        http_code = 500
    }
    response.send(http_code, getPage(message));
}
 
// register the HTTP handler
http.onrequest(requestHandler)
