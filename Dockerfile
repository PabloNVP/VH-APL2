FROM ubuntu:24.10 AS build

# Crear el directorio /apl donde se copiarán los archivos
RUN mkdir -p /apl

# Establecer el directorio de trabajo
WORKDIR /apl

# Copiar los archivos de los ejercicios en el directorio de trabajo
COPY ./apl .


