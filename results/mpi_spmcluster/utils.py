from make_plots import *

def compare_over_sizes(nnodes, nworkers_per_node, tileSize):
    nworkers = nworkers_per_node * nnodes
    sizes = [1000, 2000, 4000, 6000, 8000, 10000]
    dfs = {
        size: get_dataframe(f"output_results_mpi_spmcluster_{size}size_{nnodes}nodes.csv") for size in sizes
    }
    for size, df in dfs.items():
        dfs[size] = df[(df['nworkers'] == nworkers) & (df['tileSize'] == tileSize)][['N', 'time']]
    df = pd.concat(list(dfs.values()))
    return df

def compare_over_nodes(N, nworkers_per_node, tileSize):
    nnodes = [1, 2, 4, 8]
    dfs = {
        nodes: get_dataframe(f"output_results_mpi_spmcluster_{N}size_{nodes}nodes.csv") for nodes in nnodes
    }
    if nworkers_per_node > 1:
        seq_base_df = dfs[1][(dfs[1]['nworkers'] == 1) & (dfs[1]['tileSize'] == tileSize)][['nworkers', 'time']]
    else:
        seq_base_df = pd.DataFrame()
    for nodes, df in dfs.items():
        nworkers = nodes * nworkers_per_node
        new_df = df[(df['nworkers'] == nworkers) & (df['tileSize'] == tileSize)][['nworkers', 'time']]
        seq_base_df = pd.concat([seq_base_df, new_df])
    return seq_base_df
