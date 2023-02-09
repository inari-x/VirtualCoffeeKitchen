To start the Docker-Container, follow this tutorial from "Build an image": 
https://docs.docker.com/language/python/build-images/
https://docs.docker.com/language/python/run-containers/

From your built image, run ```$ docker run -d -p 8000:5000 python-docker```. Your app will now run on localhost:8000 in your browser. 

To run locally, run ```$ python3 -m venv .venv
$ source .venv/bin/activate``` to activate the environment you are working in. 
Next, run ```$ pip3 install -r requirements.txt``` to install Flask and its dependencies.
Then, run ```$ python3 kitchen.py``` and open "localhost:5000" in your browser. 


