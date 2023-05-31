# MultiBrowser - an URL visitor/hitter bot
Sample application created in Qt/C++ as a proof of concept on how to request
multiple HTTP links simultaneously and optionally proxified.


Usage
-----

- Enter a list of **fully-qualified links** (only http:// or https:// allowed),
separated by new lines. Or use the button to Load one list from a .txt file.

- Optionally, enter a list of proxies (either http:// or https:// or socks://),
separated by new lines as well.
Be sure to fully-qualify them, including authentication details if required.
E.g.: ` http://usr:pwd@localhost:8080 `. **Do not forget the port. It is mandatory**.

- Set a number of running threads. The allowed maximum is 16.
This is a big modifier in terms of CPU and memory.

- Set a 'cooldown' interval, in seconds, with a maximum of 60. This gives the
remote server a little time to 'breath'.
The actual interval is randomized between 1 and the amount selected.
When not using proxies, using 0 (or a small value) can give you a temporary ban
in the form of connection refusals, timeouts or 429s. **Use with care**.

- Pick the request approach, either Browser or HTTP.
1. The Browser approach visits each link with a headless browser engine. In terms
of 'not looking like a bot', this is the best option, but its downside is that
it uses way more memory and it's considerably slower.
2. The HTTP approach operates by sending plain HTTP requests. Ultimately, if the
server is 'paying attention', the simplified exchange and the single-resource
requests could raise some flags. However, as expected, this approach is quite
fast and uses the lowest possible amount of memory.

- Hit Run, keep an eye on the stats or go to do something more interesting. =)

- Hit Stop anytime. Give the program a while to stop all the running threads.


ToDo's
------

- [ ] Spoof User-Agent strings
- [ ] Inject JS code on demand


<br><br>
_This README file is under construction._
