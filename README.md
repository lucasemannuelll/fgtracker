# fgtracker

Ferramenta de linha de comando para registrar sessões de arremesso no basquete e acompanhar o aproveitamento ao longo do tempo.

Os dados são armazenados localmente em SQLite e podem ser consultados posteriormente através de relatórios no terminal.

## O que faz

* Registra sessões de treino através de um prompt interativo no terminal.
* Armazena os dados em um banco SQLite local (`fgtracker.db`).
* Gera relatórios pela linha de comando usando filtros e opções.
* Mostra estatísticas de:

  * FGM (cestas convertidas)
  * FGA (tentativas)
  * FG% (aproveitamento)

## Dependências

Para compilar o projeto é necessário ter:

* GCC ou Clang
* SQLite3
* Arquivos de desenvolvimento do SQLite (`libsqlite3-dev` em distribuições Debian/Ubuntu)

## Bibliotecas incluídas

O repositório inclui cópias locais de algumas bibliotecas utilizadas pelo projeto:

* `linenoise` para o prompt interativo do `fgctl`.
* `argtable3` para o processamento de argumentos de linha de comando do `fgreport`.

Os arquivos dessas bibliotecas estão incluídos em `vendor/` e são compilados junto com o projeto.

## Compilação

### Com Just

```bash
just build
```

Os binários serão gerados em `build/`.

### Manualmente

Crie o diretório de build:

```bash
mkdir -p build
```

Compile o registrador de sessões:

```bash
gcc -Wall -Wextra -O3 -o build/fgctl fgctl.c db.c vendor/linenoise.c -lsqlite3 -lm
```

Compile o gerador de relatórios:

```bash
gcc -Wall -Wextra -O3 -o build/fgreport fgreport.c db.c vendor/argtable3.c -lsqlite3 -lm
```

## Uso

### fgctl

Registra novas sessões de treino através de um prompt interativo.

```bash
./build/fgctl
```

### fgreport

Consulta o banco de dados e gera relatórios.

```bash
./build/fgreport
```

## Observações

* O banco de dados é criado automaticamente quando necessário.
* Todos os dados permanecem armazenados localmente.
* Não há sincronização ou integração com serviços externos.
