# fgtracker
Um par de utilitários de linha de comando para registrar e analisar sessões de arremesso (FG% — Field Goal Percentage).

- **fgctl**: gerencia as sessões (adicionar, listar, editar, excluir).
- **fgreport**: gera relatórios (histórico, estatísticas e histograma).

Os dados são armazenados em um banco SQLite local (`stats.db`).

## Requisitos

- Um compilador C (ex: `cc`/`gcc`/`clang`)
- SQLite3 (biblioteca de desenvolvimento)
- Opcional: `make` ou `just` para build/instalação

## Compilando

Com `make`:

```sh
make build
```

Ou com `just`:

```sh
just build
```

Os binários são gerados em `build/`.

## Instalando

Por padrão instala em `~/.local/bin` (sem precisar de `sudo`):

```sh
make install
# ou
just install
```

Para outro prefixo:

```sh
make install PREFIX=/usr/local
just PREFIX=/usr/local install
```

Para desinstalar:

```sh
make uninstall
just uninstall
```

## Uso

### fgctl

Menu interativo para gerenciar as sessões:

```sh
fgctl
```

Opções do menu interativo:

```sh
=== fgctl ===
  1. Adicionar sessão
  2. Listar sessões
  3. Editar sessão
  4. Excluir sessão
  5. Sair
  > 
```

### fgreport

Opções disponíveis no CLI:

| Opção          | Descrição                                                   |
| -------------- | ----------------------------------------------------------- |
| `--history`    | Lista as sessões (mais antigas primeiro)                    |
| `--stats`      | Estatísticas gerais (melhor/pior sessão, médias, tendência) |
| `--histogram`  | Histograma de FG% em intervalos de 10%                      |
| `--last N`     | Mostra apenas as últimas N sessões (com `--history`)        |
| `--week`       | Filtra pelos últimos 7 dias                                 |
| `--month`      | Filtra pelos últimos 30 dias                                |
| `--year`       | Filtra pelos últimos 365 dias                               |
| `-h`, `--help` | Mostra a ajuda                                              |

Exemplos:

```sh
fgreport --history --last 10
fgreport --stats --month
fgreport --histogram --week
```

> Obs: use apenas um dos filtros de tempo (`--week`, `--month`, `--year`) por vez.

## Dados de teste (opcional)

O script `seed.py` popula o banco com sessões aleatórias dos últimos 365 dias:

```sh
python3 seed.py
```

## Estrutura do projeto

```
db.c / db.h          # camada de acesso ao banco SQLite
models.h             # struct Session e caminho do banco (DB_PATH)
fgctl.c              # CLI interativa de gerenciamento
fgreport.c           # CLI de relatórios
seed.py              # gerador de dados de teste
Makefile.txt         # build/install via make (renomeie para Makefile)
Justfile.txt         # build/install via just (renomeie para Justfile)
```

## Licença

Este projeto não possui licença definida.
