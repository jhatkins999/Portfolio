import random
from BaseAI import BaseAI
import time
import math

class IntelligentAgent(BaseAI):
    def getMove(self, grid):
        # Selects a random move and returns it
        start_time = time.process_time()
        moveset = grid.getAvailableMoves()
        cells = grid.getAvailableCells()
        int_moves = [move[0] for move in moveset]
        if 0 in int_moves and len(cells) > 12:
            return 0
        elif 2 in int_moves and len(cells) > 12:
            return 2
        elif 3 in int_moves and len(cells) > 12:
            return 3
        else:
            best_move = None
            best_score = 0
            for move in moveset:
                grid2 = grid.clone()
                grid2.move(move[0])
                score = minimax(grid2, .19 - time.process_time() + start_time, 0, 2, 0, 10000, 0)
                if score > best_score:
                    best_move = move[0]
                    best_score = score               
            return best_move
    
def minimax(grid, time_remaining, depth, turn, alpha, beta, tile):
        move_time = time.process_time()
        if time_remaining <= 0 or depth == 8:
            return math.log2(grid.getMaxTile()) + heuristicValue(grid) # Add heuristic value here
        if turn == 1:
            max_score = 0
            moves = grid.getAvailableMoves()
            for move in moves:
                grid2 = grid.clone()
                grid2.move(move[0])
                score = minimax(grid2, time_remaining - time.process_time() + move_time, 
                                depth+1, 2, alpha, beta, tile)
                alpha = max(alpha, score)
                max_score = max(max_score, score)
                if alpha >= beta:
                    break
            return max_score
                
        elif turn == 2:
            two_score = minimax(grid, time_remaining - time.process_time() + move_time, 
                                 depth+1, 0, alpha, beta, 2)
            four_score = minimax(grid, time_remaining - time.process_time() + move_time, 
                                 depth+1, 0, alpha, beta, 4)
            return .9*two_score + .1*four_score                
            
        else:
            cells = grid.getAvailableCells()
            min_score = 100000
            for cell in cells:
                grid3 = grid.clone()
                grid3.setCellValue(cell, tile)
                score2 = minimax(grid3, time_remaining - time.process_time() + move_time, 
                                 depth+1, 1, alpha, beta, tile)
                beta = min(beta, score2)
                min_score = min(min_score, score2)
                if alpha >= beta:
                    break
            return min_score
        
def heuristicValue(grid):
        w1 = 1
        w2 = 1
        w3 = 1
        return w1*heuristic1(grid) + w2*heuristic2(grid)
    
def heuristic1(grid):
    #Monotonic Rows
        hscore1 = 0
        hscore2 = 0
        for row in grid.map:
            sorted_row = row[:]
            sorted_row.sort()
            sorted_row2 = sorted_row[:]
            sorted_row2.reverse()
            #Calculate the distance from the proper spot in the row weighted by the log of the tile value
            for val in row:
                if val == 0:
                    continue
                hscore1 += (math.log2(val)*(row.index(val) - sorted_row.index(val)))**2
                hscore2 += (math.log2(val)*(row.index(val) - sorted_row2.index(val)))**2
        return max(500 - hscore1, 500 - hscore2)/500
            
def heuristic2(grid):
    #Empty spaces
        return len(grid.getAvailableCells())/16
        
        
    