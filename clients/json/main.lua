
function onConnection(client,...)
	
	INFO("Connection of a new client json")
  
  function client:onRead(file)
  
    if file == "" and client.protocol == "HTTP" then
      return "index.html"
    end
  end
  
	function client:onMessage(data)
    INFO("New message from json : ")
    INFO("toJSON : ", mona:toJSON(data))

    return data
	end
end