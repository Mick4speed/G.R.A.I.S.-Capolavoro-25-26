import pandas as pd
import sqlite3

parquet_file = 'data.parquet'
sqlite_file = 'data.sqlite3'
table_name = 'corpus'

db_conn = sqlite3.connect(sqlite_file)
df = pd.read_parquet(parquet_file)
df.to_sql(table_name, db_conn, if_exists='replace', index=False)