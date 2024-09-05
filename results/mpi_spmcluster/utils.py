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

def compare_over_fixed_nodes(nnodes, tileSize):
    sizes = [1000, 2000, 4000, 6000, 8000, 10000]
    dfs = {
        N: get_dataframe(f"output_results_mpi_spmcluster_{N}size_{nnodes}nodes.csv") for N in sizes
    }
    dfs = {
        N: df[df['tileSize'] == tileSize][['nworkers', 'time']] for N, df in dfs.items()
    }
    vals = {}
    workers_set = False
    for N, df in dfs.items():
        if not workers_set:
            vals['nworkers'] = list(df['nworkers'])
            workers_set = True
        vals[str(N)] = list(df['time'])
    #for N, val in vals.items():
    #    print(f"{N} => {val}")
    df = pd.DataFrame(vals)
    return df

def scalability_over_sizes_fixed_wpn(nworkers_per_node, tileSize):
    base_dfs = {
        N: compare_over_nodes(N, nworkers_per_node, tileSize) for N in [1000, 2000, 4000, 6000, 8000, 10000]
    }
    df_dict = {'nworkers': [nworkers_per_node * item for item in [1, 2, 4, 8]]}
    if nworkers_per_node > 1:
        df_dict['nworkers'] = [1] + df_dict['nworkers']
    for N, df in base_dfs.items():
        df_dict[str(N)] = list(df['time'])
    for key, vals in df_dict.items():
        print(f"{key} => {vals}")
    df = pd.DataFrame(df_dict)
    scal_df = df.iloc[0] / df
    scal_df['nworkers'] = df_dict['nworkers']
    eff_df = scal_df.copy()
    for i in range(len(eff_df)):
        eff_df.iloc[i] = eff_df.iloc[i] / df_dict['nworkers'][i] * 100
    eff_df['nworkers'] = df_dict['nworkers']
    return df, scal_df, eff_df

def scalability_over_sizes_fixed_nodes(nnodes, tileSize):
    df = compare_over_fixed_nodes(nnodes, tileSize)
    nworkers = list(df['nworkers'])
    scal_df = df.iloc[0] / df
    scal_df['nworkers'] = nworkers
    eff_df = scal_df.copy()
    for i in range(len(eff_df)):
        eff_df.iloc[i] = eff_df.iloc[i] / nworkers[i] * 100
    eff_df['nworkers'] = nworkers
    return df, scal_df, eff_df

def weak_scalability(nworkers_per_node, tileSize):
    sizes = [1000, 2000, 4000, 8000]
    nodes = [1, 2, 4, 8]
    workers = [nworkers_per_node * nnodes for nnodes in nodes]
    df_ws = pd.DataFrame()
    for N, nnodes, nworkers in zip(sizes, nodes, workers):
        df = get_dataframe(f"output_results_mpi_spmcluster_{N}size_{nnodes}nodes.csv")
        df = df[(df['policy'] == 1) & (df['tileSize'] == tileSize) & (df['nworkers'] == nworkers)]
        df_ws = pd.concat([df_ws, df])[['N', 'nworkers', 'time']]
    df_ws['scalability'] = df_ws['time'] / (df_ws['nworkers']**2)
    return df_ws

def weak_scalability_comparison(tileSize):
    df_dict = {'N': [1000, 2000, 4000, 8000]}
    for nworkers_per_node in [1, 2, 4, 8]:
        df_ws = weak_scalability(nworkers_per_node, tileSize)
        df_dict[str(nworkers_per_node)] = list(df_ws['time'])
    df = pd.DataFrame(df_dict)
    return df
