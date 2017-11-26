# Database structure

The tables are:

**settings**:
Used for remembering settings and for transferring them between web pages and modules.
One row per setting.

**currentvalues**:
The latest outputs from all modules are registered here.
One row per output.

**timeseries**:
This table keeps a complete history of all or selected outputs, for plotting and analysis.
One row per time stamp.
One column per output.
Each row in the currentvalues table that matches a column in the timeseries table will be stored historically.

The file home_controle.sql can be executed to create the tables in MariaDb or MySql.
