# CS 523 - Project 3
# How to run: python ga.py pop_size num_of_generations longevity/biomass

import math
import operator
import sys
import numpy as np
import os
import random
import matplotlib.pyplot as plt
import collections

# GA Individual class
class GAIndividual:
    def __init__(self, genome, fitness):
        self.genome = genome
        self.fitness = fitness

    def __lt__(self, other):
        return self.fitness < other.fitness


# Returns the biomass as fitness
def biomass_fitness_func(p_m, firefighters_number):
    global p2tree
   
    command = "./ff.out  " + str(p_m) + " " + str(firefighters_number) + " " + str(p2tree) + "> res.txt"
    os.system(command)
    res_file = open("res.txt", "r")
    result = res_file.read()
    line = result.split("\n")[1]
    scores = line.split(",")
    biomass = scores[1].split(":")[1]
    return int(biomass)


# Returns the longevity as fitness
def longevity_fitness_func(p_m, firefighters_number):
   
    command = "./ff.out  " + str(p_m) + " " + str(firefighters_number) + " 0 0.0009 > res.txt"
    os.system(command)
    res_file = open("res.txt", "r")
    result = res_file.read()
    line = result.split("\n")[1]
    scores = line.split(",")
    longevity = scores[0].split(":")[1]
    return int(longevity)


# Returns the fitness value
def fitness(p_m, firefighters_number):
    global fitness_func
    if fitness_func is 'longevity':
        return longevity_fitness_func(p_m, firefighters_number)
    else:
        return biomass_fitness_func(p_m, firefighters_number)


# Returns the average fitness value
def avg_score(population, pop_size):
    return sum(pop.fitness for pop in population)/pop_size


def should_mutate():
    return np.random.randint(1, 100) <= mutation_rate


# Mutates the population. It increases/decreases the genome (i.e. probability)
def mutate(individual):
    global increase_rate
    fraction = random.uniform(0, 0.1)
    if np.random.randint(1, 100) <= increase_rate:
        if individual.genome + fraction > 0 and individual.genome + fraction < 1:
            individual.genome = individual.genome + fraction
    else:
        if individual.genome - fraction > 0 and individual.genome - fraction < 1:
            individual.genome = individual.genome - fraction
    return individual
  
# Generates a new population
def create_a_population(pop_size):
    population = []
    for i in range(pop_size):
        genome = random.uniform(0, 1)
        fitness = 0
        indv = GAIndividual(genome, fitness)
        population.append(indv)
    return population


''' Random selection method 
    Replace bottom half of population 
    with random individuals from the top % half '''
def random_selection(population, pop_size):
    # Sort population by fitness (in ascending order)
    population = np.sort(population)
    # flooring in case the population size is odd
    cutoff = int(math.floor(pop_size/2))
    for i in range(cutoff):
        index = np.random.randint(cutoff)
        population[i].genome = population[cutoff + index].genome
        population[i].fitness = population[cutoff + index].fitness
    return population


def get_top_two(population, pop_size):
    population = np.sort(population)    
    return population[pop_size-1].genome, population[pop_size-2].genome


def plot_landscape():
    global landscape
    global fitness_func
    global p2tree
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_xlabel("$p_m$", fontsize=18)
    ax.set_ylabel(fitness_func, fontsize=18)
    plt.plot(list(landscape.keys()), list(landscape.values()), marker='^')
    savefile = './two_trees' + str(p2tree) + '.png'
    fig.savefig(savefile)
    #plt.show()
    plt.close(fig)

def update_fitness_landscape(population, pop_size):
    global landscape
    for indv in population:
        landscape[indv.genome] = indv.fitness


## Plot firefighters number vs longevity or firefighters number vs biomass
def plot_ff_fitness():
    global fitness_func
    fitness_list = []
    p_m = 0.997
    #firefighters_number = range(0,1050,50)
    ff = 0
    #for ff in firefighters_number:
    print "firefighters number:", ff
    fitness_list.append(fitness(p_m, ff))
    # plot the result
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_xlabel("firefighters number", fontsize=18)
    ax.set_ylabel(fitness_func, fontsize=18)
    plt.plot(firefighters_number, fitness_list, marker='^')
    plt.show()

## GA that tries to find the maximum value
def GA(pop_size, mutation_rate, num_generations, fitness):
    global landscape
    population = create_a_population(pop_size)
    print "> initial population generated!"
    for generation in range(num_generations):
        print "========================"
        print "> Mutating population..."
        # Mutate population
        for i in range(pop_size):
            if should_mutate(): # mutate
                population[i] = mutate(population[i])

        print "> Evaluating fitness..."
        # Evaluate fitness
        for i in range(pop_size):
            population[i].fitness = fitness(population[i].genome, 0)

        print "> Generating the next generation..."
        # Replace bottom half of population
        # with random individuals from the top % half
        population = random_selection(population, pop_size)

        top_score = max(pop.fitness for pop in population)
        avg_score = sum(pop.fitness for pop in population)/pop_size
        print "> Generation " + str(generation+1) + " created!"
        print "> Avg fitness in this generation: " + str(avg_score)
        print "> Top fitness in this generation: " + str(top_score)
        update_fitness_landscape(population, pop_size)

    print "top two p_m:", get_top_two(population, pop_size)



''' ******************* MAIN BODY ******************* '''
pop_size = int(sys.argv[1])
num_generations = int(sys.argv[2])
landscape = {}
mutation_rate = 100 # out of 100
increase_rate = 50 # out of 100
p2tree = 0.05

fitness_func = 'longevity'
if sys.argv[3] == 'longevity' or sys.argv[3] == 'biomass':
    fitness_func = sys.argv[3]
    ## Run GA to evolve p_m
    GA(pop_size, mutation_rate, num_generations, fitness)
    ## Plot landscape
    landscape = collections.OrderedDict(sorted(landscape.items()))
    plot_landscape()
    ## Plot firefighters number vs longevity and firefighters number vs biomass
    #fitness_func = 'biomass'
    #plot_ff_fitness()
    #fitness_func = 'longevity'
    #plot_ff_fitness()
else:
    print "invalid arguments!"



