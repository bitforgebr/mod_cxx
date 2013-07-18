# mod_cxx

mod_cxx é um framework para facilitar o desenvolvimento de aplicações
Web em C++. Atualmente, mod_cxx é um módulo do servidor Apache que
abstrai a interação com o servidor. Isso facilita o desenvolvimento,
que, essencialmente, pode ser resumido no tratamento das requisições
HTTP.

## Dependências

* libbf (https://github.com/bitforgebr/libbf)
* Apache2 Server
* apr

## Build

Instale as dependências descritas acima. O nome dos packages
devem variar em cada distribuição linux, portanto, é recomendado
que use a funcionalidade de pesquisa do gerenciador de pacotes
da sua distribuição. Por exemplo, para instalar, no Fedora, o servidor Apache2
e seus arquivos de desenvolvimento, basta executar o seguinte comando:

`yum install httpd httpd-devel`

Para saber qual é o nome do package que contém a biblioteca apr, basta executar
o seguinte comando:

`yum search apr`

Esse comando retorna uma lista de pacotes, sendo os pacotes apr e apr-devel 
de interesse aqui. Para instalá-los basta:

`yum install apr apr-devel`

Veja que sempre(?) instala-se dois pacotes, um com os binários da biblioteca
e outro pacote -devel, que contém os arquivos headers da biblioteca.

Dado que os frameworks listados acima estão devidamente instalados,
o build pode ser feito usando o CMake de acordo com os seguintes passos:

1. Crie uma basta chamada build, na raiz do projeto:
    `mkdir build`
2. Dentro da pasta build, execute o CMake, com o seguinte comando:
    `cmake ..`
3. Faça o build usando o make, com o seguinte comando:
    `make`

Uma vez que o módulo foi construído, é possível adicioná-lo ao diretório
de módulos do Apache. Esse diretório varia de distribuição para distribuição.

No Fedora, esse diretório fica em /etc/httpd/modules

No openSUSE, esse diretório fica em /srv/www/modules

Já com o mod_cxx no diretório de módulos do Apache, é preciso criar um arquivo
de configuração e adicioná-lo em um diretório reservado para tais arquivos.
Nesse arquivo, o módulo é configurado de modo que o Apache sabe que tipo
de requisições devem ser encaminhadas ao módulo.

Na pasta debug, encontrada na raiz do projeto, há um arquivo, 99_mod_cxx.conf,
que pode ser usado para configurar o módulo mod_cxx dentro do Apache. 
Assim, copie esse arquivo para a pasta específica para arquivos de configuração.
Essa pasta também varia de acordo com a distribuição:

No Fedora, esse diretório fica em /etc/httpd/conf.d

No openSUSE, esse diretório fica em /etc/apache2/conf.d

Reinicie o Apache:

No Fedora, `service httpd restart`

No openSUSE, `service apache2 restart`

Pronto, o módulo está instalado.