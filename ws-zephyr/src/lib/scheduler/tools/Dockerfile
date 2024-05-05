FROM tomeinc/b2v-lint:v2.0.0
RUN apt-get update
RUN apt-get install -y gcc g++ gfortran libopenblas-dev liblapack-dev pkg-config python3-pip python3-dev
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
