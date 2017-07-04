qtchan is a 4chan browser written with qt5.\
I started developing it because browsers suck.\
There's still a lot to do.

# Features
* browse and post (only 4chan pass right now) on 4chan
* auto update threads and auto download original file
* download all media as original filename
* tree style tabs and save state
* open files in OS's default application
* search and filter

## Using 4chan pass to post
Create a file ~/.config/qtchan/settings like below. Look at your browser cookies for values.
```
{
	"cookies":{
		"4chan_pass":"your 4chanpass",
		"pass_id":"your pass id",
		"pass_enabled":"1"
	}
}
```

![qtchan ss](https://i.abcdn.co/qtchan.png)

## TODO
### MAJOR
* captcha
* unread, read, (You) display
* notifications
* instructions

### MINOR
* show loading state
* search/filter by different post fields (right now combo of sub and com)
* more settings
* themes
