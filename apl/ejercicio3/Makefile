# Makefile

CXX = g++
CXXFLAGS = -Wall -g
TARGET_CENTRAL = proceso_central
TARGET_SENSOR = proceso_sensor

# Regla por defecto
all: $(TARGET_CENTRAL) $(TARGET_SENSOR)

# Compilación del proceso central
$(TARGET_CENTRAL): proceso_central.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Compilación del proceso sensor
$(TARGET_SENSOR): proceso_sensor.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Limpiar archivos generados
clean:
	rm -f $(TARGET_CENTRAL) $(TARGET_SENSOR)
