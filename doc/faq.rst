
.. image:: githubBlack.png
  :align: right
  :target: https://github.com/MonaSolutions/MonaServer

FAQ
##############################

Here are listed answers for frequently asked questions.

.. contents:: Table of Contents

What are the advantages of RTMFP Protocol?
*******************************************

**RTMFP** is the most powerfull web protocol for these principal reasons :

 - permits broadcast and multicast (virtual and real),
 - use UDP to avoid congestion transfert (well suited for live communication),
 - it has a reliable mode for UDP,
 - includes P2P channel communication,
 - and all integrated in a well designed implementation technology,


Why the `cirrus sample`_ doesn't work?
*******************************************

Because cirrus server includes the following script server:

.. code-block:: lua

	function onConnection(client,...)
		function client:relay(targetId,...)
			target = mona.clients(targetId)
			if not target then
				error("client '"..targetId.."' not found")
				return
			end
			target.writer:writeInvocation("onRelay",self.id,...)
			target.writer:flush(true)
		end
	end

Adding it for your *www/main.lua* file will make working the `cirrus sample`_.

.. note:: the `cirrus sample`_ requires some python scripts to exchange user names, it makes the thing complex whereas it could be easily to replace with some LUA code line. I guess you to keep an eye on the *meeting sample* of the `Samples <./samples.html>`_ page which does the same thing but without other external dependencies.

.. _`cirrus sample`: http://labs.adobe.com/technologies/cirrus/samples/
