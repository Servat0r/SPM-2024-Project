from make_plots import *

def main(folder: str, xlabel: str, title: str, xticks: list = None, yticks: list = None):
    for filename in os.listdir(folder):
        if filename.endswith('.csv'):
            df = get_dataframe(os.path.join(folder, filename))
            # Speedup
            make_speedup_plot(
                df, 'line', xlabel=xlabel, title='Speedup', xticks=xticks, yticks=yticks,
                savepath=os.path.join(folder, f"{filename.split('.')[0]}_speedup.png")
            )
            plt.close()
            # Efficiency
            make_efficiency_plot(
                df, 'line', xlabel=xlabel, title='Efficiency', xticks=xticks, yticks=yticks,
                savepath=os.path.join(folder, f"{filename.split('.')[0]}_efficiency.png")
            )
            plt.close()
