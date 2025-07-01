### BASE SETUP
FROM continuumio/miniconda3:latest
RUN pip install flask
RUN apt-get update
RUN apt-get install -y --no-install-recommends libatlas-base-dev gfortran nginx supervisor
RUN pip install  --no-binary pyuwsgi pyuwsgi

### Application-specific Setup
RUN conda install -y -c conda-forge ambertools
RUN wget https://github.com/m3g/packmol/archive/refs/tags/v20.14.2.zip
RUN apt-get install unzip
RUN unzip v20.14.2.zip
WORKDIR /packmol-20.14.2/
RUN apt-get install make
RUN make
RUN mv packmol /bin/
WORKDIR /
RUN rm -r packmol-20.14.2 v20.14.2.zip
COPY Application/ /app
WORKDIR /app

ARG CACHEBUST=1
### Initialize uWSGI server and run upon container startup
CMD ["uwsgi","--http","0.0.0.0:8531","--master","-p","4","-w","app:app"]
EXPOSE 8531