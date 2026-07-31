# fgtracker

Ferramenta de linha de comando para registrar sessões de arremesso no basquete e acompanhar o aproveitamento ao longo do tempo.

Os dados são armazenados localmente em SQLite e podem ser consultados posteriormente através de relatórios no terminal.

## Funcionalidades

### fgctl - Gerenciador interativo de sessões

- **Adicionar sessão**: Registre novas sessões de treino informando FGM (cestas convertidas) e FGA (tentativas)
- **Listar sessões**: Visualize todas as sessões registradas em ordem cronológica
- **Editar sessão**: Atualize os dados de uma sessão existente pelo ID
- **Excluir sessão**: Remova sessões indesejadas com confirmação

### fgreport - Gerador de relatórios

- **Histórico completo**: Liste todas as sessões com datas e aproveitamentos
- **Estatísticas da carreira**: Visualize métricas como total de arremessos, média de acertos, melhor/pior sessão e desvio padrão
- **Histograma de FG%**: Distribuição do aproveitamento em intervalos de 10%
- **Filtros temporais**: Analise dados das últimas semanas, meses ou ano
- **Últimas N sessões**: Foque nos registros mais recentes

## Estrutura dos dados

Cada sessão registrada contém:

- **ID**: Identificador único (gerado automaticamente)
- **Data/hora**: Timestamp do registro
- **FGM**: Field Goals Made (cestas convertidas)
- **FGA**: Field Goals Attempted (tentativas)
- **FG%**: Aproveitamento calculado automaticamente

## Dependências

Para compilar o projeto é necessário ter:

- GCC ou Clang
- SQLite3
- Arquivos de desenvolvimento do SQLite (`libsqlite3-dev` em distribuições Debian/Ubuntu)

### Instalando dependências no Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev
```

### Instalando dependências no macOS

```bash
brew install sqlite3
```

## Compilação

### Usando Just (recomendado)

```bash
just build
```

### Compilação manual

Crie o diretório de build:

```bash
mkdir -p build
```

Compile o gerenciador de sessões:

```bash
gcc -Wall -Wextra -O3 -o build/fgctl fgctl.c db.c -lsqlite3 -lm
```

Compile o gerador de relatórios:

```bash
gcc -Wall -Wextra -O3 -o build/fgreport fgreport.c db.c -lsqlite3 -lm
```

## Uso

### fgctl - Gerenciador interativo

Execute o programa e siga o menu interativo:

```bash
./build/fgctl
```

Menu disponível:

```
=== fgctl ===
  1. Adicionar sessão
  2. Listar sessões
  3. Editar sessão
  4. Excluir sessão
  5. Sair
  > 
```

### fgreport - Gerador de relatórios

Execute com diferentes opções para gerar relatórios personalizados:

```bash
# Mostrar todas as sessões
./build/fgreport --history

# Mostrar estatísticas da carreira
./build/fgreport --stats

# Mostrar histograma de aproveitamento
./build/fgreport --histogram

# Combinar opções
./build/fgreport --history --stats --histogram

# Filtrar por período
./build/fgreport --history --week     # Últimos 7 dias
./build/fgreport --stats --month      # Último mês
./build/fgreport --histogram --year   # Último ano

# Mostrar apenas as últimas 10 sessões
./build/fgreport --history --last 10

# Ajuda
./build/fgreport --help
```

#### Opções disponíveis

| Opção | Descrição |
|-------|-----------|
| `--history` | Lista todas as sessões (mais antigas primeiro) |
| `--stats` | Exibe estatísticas da carreira |
| `--histogram` | Mostra distribuição do FG% em bins de 10% |
| `--last N` | Mostra apenas as últimas N sessões (com `--history`) |
| `--week` | Filtra pelos últimos 7 dias |
| `--month` | Filtra pelo último mês |
| `--year` | Filtra pelo último ano |
| `-h, --help` | Exibe a ajuda |

#### Exemplo de saída

**Relatório de histórico:**

```
  ID   | Data/hora           | FGM/FGA | FG%
  -----|---------------------|---------|-------
  1    | 2026-01-15 14:30:00 |   5/10  |  50.0%
  2    | 2026-01-16 10:15:00 |   8/12  |  66.7%
```

**Relatório estatístico:**

```
=== Career Stats (sessions: 25) ===
  Total FGM: 150
  Total FGA: 280
  Overall FG%: 53.6%
  Best session: [12] 80.0%
  Worst session: [5] 20.0%
  Avg makes/session: 6.0
  Avg attempts/session: 11.2
  Std dev (FG%): 12.34%
  Std dev last 10: 8.45%
```

**Histograma:**

```
=== FG% Histogram (10% bins) ===
   0%–  9% : █ (2)
  10%– 19% : ██ (3)
  20%– 29% : ████ (5)
  30%– 39% : ██████ (7)
  40%– 49% : ███████████ (12)
  50%– 59% : ████████████████ (18)
  60%– 69% : ███████████ (12)
  70%– 79% : ██████ (7)
  80%– 89% : ███ (4)
  90%–100% : █ (1)
```

## Estrutura do projeto

```
fgtracker/
├── db.c              # Operações com banco de dados SQLite
├── db.h              # Interface do banco de dados
├── models.h          # Definição da estrutura Session
├── fgctl.c           # Gerenciador interativo de sessões
├── fgreport.c        # Gerador de relatórios
├── stats.db          # Banco de dados SQLite (criado automaticamente)
└── README.md         # Este arquivo
```

## Observações

- O banco de dados (`stats.db`) é criado automaticamente na primeira execução
- Todos os dados permanecem armazenados localmente
- Não há sincronização ou integração com serviços externos
- O campo `datetime` é preenchido automaticamente com a data/hora local do sistema

## Licença

Este projeto é de uso livre para fins educacionais e pessoais.
