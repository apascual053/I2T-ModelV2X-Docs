#!/bin/bash

# Crear directorio temporal
mkdir -p ../temp

# Copiar los archivos generados de la documentación a la carpeta temporal
cp -r build/html/* ../temp/

# Cambiar a la rama gh-pages
git checkout gh-pages

# Copiar los archivos de la carpeta temporal al directorio actual
cp -r ../temp/* .

# Agregar los cambios a git
git add .

# Realizar commit con un mensaje
git commit -m "gh-paging publishing"

# Empujar los cambios a la rama gh-pages
git push origin gh-pages

# Eliminar el directorio temporal
rm -r ../temp/

# Finalizar
echo "Despliegue completado correctamente."

# Cambiar a la rama main
git checkout main