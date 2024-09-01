# Plots for Speedup, Strong/Weak Scalability, Efficiency
import matplotlib.pyplot as plt
import pandas as pd
import os
import numpy as np


def cls(): os.system('cls')
def clear(): os.system('clear')

def get_dataframe(filename: str):
    return pd.read_csv(filename)

def select_speedup_data(
        df: pd.DataFrame, N: int, tileSize: int, policy: int,
        chunkSize: int = 1, drop_columns: bool = True,
        sequential_time: int = None, base_df: pd.DataFrame = None, filter = False,
):
    if filter:
        filtered_parallel_df = df[
            (df['N'] == N) &
            (df['tileSize'] == tileSize) &
            (df['policy'] == policy) &
            (df['nworkers'] > 0)
        ]
        if policy == 3:
            filtered_parallel_df = filtered_parallel_df[filtered_parallel_df['chunkSize'] == chunkSize]
    else:
        filtered_parallel_df = df
    if sequential_time is None:
        filtered_sequential_df = df[
            (df['N'] == N) &
            (df['tileSize'] == tileSize) &
            (df['policy'] == 0)
        ]
        if drop_columns:
            if 'chunkSize' in filtered_sequential_df:
                filtered_sequential_df = filtered_sequential_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
            else:
                filtered_sequential_df = filtered_sequential_df.drop(columns=['N', 'tileSize', 'policy'])
        sequential_time = list(filtered_sequential_df['time'])[0]
    if filter and drop_columns:
        if 'chunkSize' in filtered_parallel_df:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
        else:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['N', 'tileSize', 'policy'])
        if 'checksum' in filtered_parallel_df:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['checksum'])
        if 'nnodes' in filtered_parallel_df:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['nnodes'])
        if 'MPITime' in filtered_parallel_df:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['MPITime'])
    speedup_df = filtered_parallel_df
    #speedup_df.loc[len(speedup_df)] = {'nworkers': 1, 'time': sequential_time}
    speedup_df = speedup_df.sort_values(by='nworkers')
    speedup_df['speedup'] = speedup_df['time'].apply(lambda x : sequential_time / x)
    return speedup_df

def select_strong_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, drop_columns: bool = True, filter = False,
):
    if filter:
        if 'policy' in df:
            filtered_df = df[
                (df['N'] == N) &
                (df['tileSize'] == tileSize) &
                (df['policy'] == policy)
            ]
        else:
            filtered_df = df[
                (df['N'] == N) &
                (df['tileSize'] == tileSize)
            ]
        if policy == 3:
            filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'policy' in df:
            if 'chunkSize' in filtered_df:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
            else:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'policy'])
        else:
            if 'chunkSize' in filtered_df:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'chunkSize'])
            else:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize'])
    single_thread_time = filtered_df[filtered_df['nworkers'] == 1]['time']
    print(filtered_df, single_thread_time, sep='\n\n')
    filtered_df['strong_scalability'] = filtered_df['time'].apply(lambda x : single_thread_time / x)
    return filtered_df


def select_weak_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, maxN: int = 8000, drop_columns: bool = True, filter = False,
):
    filtered_df = df[
        (df['tileSize'] == tileSize) &
        (df['policy'] == policy)
    ]
    if policy == 3:
        filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'chunkSize' in filtered_df:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy', 'chunkSize'])
        else:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy'])
    p = 2
    result_df = filtered_df[(filtered_df['N'] == N) & (filtered_df['nworkers'] == 1)]
    while (N * p <= maxN):
        newline = filtered_df[(filtered_df['N'] == p*N) & (filtered_df['nworkers'] == p)]
        if len(newline):
            result_df = pd.concat([result_df, newline])
        p += 1
    sequential_time = list(result_df[result_df['nworkers'] == 1]['time'])[0]
    result_df['weak_scalability'] = sequential_time * result_df['nworkers'] / result_df['time']
    result_df['quadratic_weak_scalability'] = result_df['weak_scalability'] * result_df['nworkers']
    return result_df


def select_quadratic_weak_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, maxN: int = 8000, drop_columns: bool = True, filter = False,
):
    filtered_df = df[
        (df['tileSize'] == tileSize) &
        (df['policy'] == policy)
    ]
    if policy == 3:
        filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'chunkSize' in filtered_df:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy', 'chunkSize'])
        else:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy'])
    p = 2
    result_df = filtered_df[(filtered_df['N'] == N) & (filtered_df['nworkers'] == 1)]
    while (N * p * p <= maxN):
        newline = filtered_df[(filtered_df['N'] == p*p*N) & (filtered_df['nworkers'] == p*p)]
        if len(newline):
            result_df = pd.concat([result_df, newline])
        p += 1
    result_df['weak_scalability'] = result_df['time'] / result_df['nworkers']
    return result_df


def select_efficiency_data(
        df: pd.DataFrame, N: int, tileSize: int, policy: int,
        chunkSize: int = 1, drop_columns: bool = True,
        sequential_time: int = None, filter = False,
):
    efficiency_df = select_speedup_data(
        df, N, tileSize, policy, chunkSize, drop_columns, sequential_time=sequential_time, filter=filter
    )
    efficiency_df['efficiency'] = efficiency_df['speedup'] / efficiency_df['nworkers'] * 100
    return efficiency_df

def round_to_significant_digits(value, digits):
    if value == 0:
        return 0
    return round(value, digits - int(np.floor(np.log10(abs(value)))) - 1)

def write_back_data(
    df: pd.DataFrame, base_filename: str,
    select_type: str, N: int, tileSize: int,
    policy: int, chunkSize: int = 1, digits: int = 4
):
    filename = f"{base_filename}_{select_type}_{N}_{policy}_{tileSize}_{chunkSize}.csv"
    for field in ['speedup', 'efficiency', 'strong_scalability', 'weak_scalability', 'time']:
        if field in df:
            df[field] = df[field].apply(lambda x: round_to_significant_digits(x, digits))
    df.to_csv(filename, index=False)

##########################################à
def make_plot(
        df: pd.DataFrame, kind: str, x: str, y: str, xlabel: str, ylabel: str,
        title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    df.plot(
        kind=kind, x=x, y=y, xlabel=xlabel, ylabel=ylabel, title=title, xticks=xticks, yticks=yticks
    )
    if save:
        plt.savefig(savepath)
    if show:
        plt.show()

def make_speedup_plot(
        df: pd.DataFrame, kind: str, xlabel: str,
        title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    make_plot(df, kind, 'nworkers', 'speedup', xlabel, 'Speedup', title, xticks, yticks, show, save, savepath)

def make_efficiency_plot(
        df: pd.DataFrame, kind: str, xlabel: str,
        title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    make_plot(df, kind, 'nworkers', 'efficiency', xlabel, 'Efficiency', title, xticks, yticks, show, save, savepath)

def make_strong_scalability_plot(
        df: pd.DataFrame, kind: str, xlabel: str,
        title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    make_plot(
        df, kind, 'nworkers', 'strong_scalability', xlabel, 'Scalability',
        title, xticks, yticks, show, save, savepath
    )

def make_weak_scalability_plot(
        df: pd.DataFrame, kind: str, xlabel: str,
        title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    make_plot(
        df, kind, 'nworkers', 'weak_scalability', xlabel, 'Scalability',
        title, xticks, yticks, show, save, savepath
    )

def make_comparison_plot(
        df_dict: dict[str, pd.DataFrame], kind: str, x: str, y: str, xlabel: str,
        ylabel: str, title: str, xticks: list = None, yticks: list = None,
        show: bool = False, save: bool = True, savepath: str = None
):
    field_dict = {}
    for value in df_dict.values():
        field_dict[x] = list(value[x])
        #df = value[[x]]
        break
    for key, value in df_dict.items():
        field_dict[key] = list(value[y])
        #df[key] = value[y]
    df = pd.DataFrame(field_dict)
    df.plot(kind=kind, x=x, xlabel=xlabel, ylabel=ylabel, title=title, xticks=xticks, yticks=yticks)
    if save:
        plt.savefig(savepath)
    if show:
        plt.show()
