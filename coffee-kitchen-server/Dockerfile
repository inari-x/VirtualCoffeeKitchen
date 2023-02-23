# syntax=docker/dockerfile:1

FROM python:3.8-slim-buster

WORKDIR /coffee-kitchen-server

ENV PARTICIPANT_COUNT=0
COPY requirements.txt requirements.txt
RUN pip3 install -r requirements.txt
RUN apt-get update && apt-get install -y curl

COPY . .
RUN chmod 0644 ./set_participant_variable.sh
CMD [./set_participant_variable.sh]

CMD ["python3", "kitchen.py", "--host=0.0.0.0"]
