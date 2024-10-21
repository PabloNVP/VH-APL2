FROM ubuntu:24.10 AS build

# Actualiza el índice de paquetes e instala build-essential
RUN apt-get update && apt-get install -y build-essential

# Establecer el directorio de trabajo y crearlo si no existe
WORKDIR /apl

# Copiar los archivos de los ejercicios en el directorio de trabajo
COPY ./apl/ .

